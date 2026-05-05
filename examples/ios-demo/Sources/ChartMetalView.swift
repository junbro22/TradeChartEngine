import SwiftUI
import MetalKit
import TradeChartEngine

// MARK: - SwiftUI wrapper

struct ChartMetalView: UIViewRepresentable {
    let chart: Chart

    func makeCoordinator() -> Coordinator {
        Coordinator(chart: chart)
    }

    func makeUIView(context: Context) -> MTKView {
        let v = MTKView(frame: .zero, device: MTLCreateSystemDefaultDevice())
        v.colorPixelFormat = .bgra8Unorm
        v.clearColor = MTLClearColorMake(0.07, 0.08, 0.11, 1.0)
        v.preferredFramesPerSecond = 60
        v.delegate = context.coordinator
        context.coordinator.attach(view: v)

        // 인터랙션
        let pinch = UIPinchGestureRecognizer(target: context.coordinator, action: #selector(Coordinator.handlePinch(_:)))
        let pan   = UIPanGestureRecognizer(target: context.coordinator, action: #selector(Coordinator.handlePan(_:)))
        let long  = UILongPressGestureRecognizer(target: context.coordinator, action: #selector(Coordinator.handleLongPress(_:)))
        long.minimumPressDuration = 0.2
        v.addGestureRecognizer(pinch)
        v.addGestureRecognizer(pan)
        v.addGestureRecognizer(long)

        return v
    }

    func updateUIView(_ uiView: MTKView, context: Context) {
        uiView.setNeedsDisplay(uiView.bounds)
    }

    // MARK: - Coordinator

    final class Coordinator: NSObject, MTKViewDelegate {
        let chart: Chart
        private var renderer: MetalRenderer?

        init(chart: Chart) { self.chart = chart }

        func attach(view: MTKView) {
            guard let device = view.device else { return }
            renderer = try? MetalRenderer(device: device, pixelFormat: view.colorPixelFormat)
        }

        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
            chart.setSize(width: size.width, height: size.height)
        }

        func draw(in view: MTKView) {
            guard let renderer,
                  let drawable = view.currentDrawable,
                  let pass = view.currentRenderPassDescriptor else { return }
            chart.setSize(width: view.drawableSize.width, height: view.drawableSize.height)
            let meshes = chart.buildFrame()
            renderer.render(meshes: meshes, drawable: drawable, descriptor: pass, viewportSize: view.drawableSize)
        }

        // 핀치: 양 → zoom-in (캔들 적게)
        @objc func handlePinch(_ g: UIPinchGestureRecognizer) {
            guard let view = g.view else { return }
            switch g.state {
            case .changed:
                let factor = Float(g.scale)
                if factor > 0 {
                    let anchor = g.location(in: view).x * view.contentScaleFactor
                    chart.zoom(factor: CGFloat(factor), anchorX: anchor)
                }
                g.scale = 1
            default: break
            }
            view.setNeedsDisplay()
        }

        // 가로 드래그: pan
        @objc func handlePan(_ g: UIPanGestureRecognizer) {
            guard let view = g.view else { return }
            switch g.state {
            case .changed:
                let translation = g.translation(in: view)
                chart.pan(deltaPixels: -translation.x * view.contentScaleFactor)
                g.setTranslation(.zero, in: view)
            default: break
            }
            view.setNeedsDisplay()
        }

        // 길게 누름: 크로스헤어 표시
        @objc func handleLongPress(_ g: UILongPressGestureRecognizer) {
            guard let view = g.view else { return }
            switch g.state {
            case .began, .changed:
                let p = g.location(in: view)
                chart.setCrosshair(x: p.x * view.contentScaleFactor,
                                   y: p.y * view.contentScaleFactor)
            case .ended, .cancelled, .failed:
                chart.clearCrosshair()
            default: break
            }
            view.setNeedsDisplay()
        }
    }
}

// MARK: - Renderer

private final class MetalRenderer {
    private let device: MTLDevice
    private let queue: MTLCommandQueue
    private let pipeline: MTLRenderPipelineState
    private let pipelineLines: MTLRenderPipelineState

    init(device: MTLDevice, pixelFormat: MTLPixelFormat) throws {
        self.device = device
        guard let q = device.makeCommandQueue() else {
            throw NSError(domain: "Renderer", code: -1)
        }
        self.queue = q

        let library = try device.makeLibrary(source: shaderSource, options: nil)
        let vfn = library.makeFunction(name: "tce_vs")
        let ffn = library.makeFunction(name: "tce_fs")

        let desc = MTLRenderPipelineDescriptor()
        desc.vertexFunction   = vfn
        desc.fragmentFunction = ffn
        desc.colorAttachments[0].pixelFormat = pixelFormat
        desc.colorAttachments[0].isBlendingEnabled = true
        desc.colorAttachments[0].sourceRGBBlendFactor = .sourceAlpha
        desc.colorAttachments[0].destinationRGBBlendFactor = .oneMinusSourceAlpha

        let vd = MTLVertexDescriptor()
        vd.attributes[0].format = .float2; vd.attributes[0].offset = 0;  vd.attributes[0].bufferIndex = 0
        vd.attributes[1].format = .float4; vd.attributes[1].offset = 8;  vd.attributes[1].bufferIndex = 0
        vd.layouts[0].stride = 24
        desc.vertexDescriptor = vd

        self.pipeline      = try device.makeRenderPipelineState(descriptor: desc)
        self.pipelineLines = try device.makeRenderPipelineState(descriptor: desc)
    }

    func render(meshes: [Mesh],
                drawable: CAMetalDrawable,
                descriptor: MTLRenderPassDescriptor,
                viewportSize: CGSize) {
        guard !meshes.isEmpty,
              let cmd = queue.makeCommandBuffer(),
              let enc = cmd.makeRenderCommandEncoder(descriptor: descriptor) else { return }

        var size = SIMD2<Float>(Float(viewportSize.width), Float(viewportSize.height))
        enc.setVertexBytes(&size, length: MemoryLayout<SIMD2<Float>>.size, index: 1)

        for mesh in meshes {
            guard !mesh.vertices.isEmpty, !mesh.indices.isEmpty else { continue }

            // setVertexBytes는 4KB 한도라 큰 메시는 MTLBuffer 필수
            let vLen = mesh.vertices.count * MemoryLayout<ChartVertex>.stride
            let vBuf: MTLBuffer? = mesh.vertices.withUnsafeBufferPointer { vb in
                guard let base = vb.baseAddress else { return nil }
                return device.makeBuffer(bytes: base, length: vLen, options: .storageModeShared)
            }

            let iLen = mesh.indices.count * MemoryLayout<UInt32>.stride
            let iBuf: MTLBuffer? = mesh.indices.withUnsafeBufferPointer { ib in
                guard let base = ib.baseAddress else { return nil }
                return device.makeBuffer(bytes: base, length: iLen, options: .storageModeShared)
            }

            guard let vBuf, let iBuf else { continue }

            let prim: MTLPrimitiveType = (mesh.primitive == .lines) ? .line : .triangle
            enc.setRenderPipelineState(prim == .line ? pipelineLines : pipeline)
            enc.setVertexBuffer(vBuf, offset: 0, index: 0)
            enc.drawIndexedPrimitives(
                type: prim,
                indexCount: mesh.indices.count,
                indexType: .uint32,
                indexBuffer: iBuf,
                indexBufferOffset: 0
            )
        }
        enc.endEncoding()
        cmd.present(drawable)
        cmd.commit()
    }
}

// MARK: - Shaders

private let shaderSource = """
#include <metal_stdlib>
using namespace metal;

struct VIn  { float2 pos [[attribute(0)]]; float4 col [[attribute(1)]]; };
struct VOut { float4 pos [[position]];     float4 col;                };

vertex VOut tce_vs(VIn in [[stage_in]],
                   constant float2& size [[buffer(1)]]) {
    // 입력은 화면 픽셀 좌표(top-left origin). NDC로 변환.
    float2 ndc;
    ndc.x =  (in.pos.x / size.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (in.pos.y / size.y) * 2.0;
    VOut out;
    out.pos = float4(ndc, 0.0, 1.0);
    out.col = in.col;
    return out;
}

fragment float4 tce_fs(VOut in [[stage_in]]) {
    return in.col;
}
"""
