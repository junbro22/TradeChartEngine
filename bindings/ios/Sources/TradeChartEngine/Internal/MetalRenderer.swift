#if os(iOS)
import MetalKit

final class MetalRenderer {
    private let device: MTLDevice
    private let queue: MTLCommandQueue
    private let pipelineTriangles: MTLRenderPipelineState
    private let pipelineLines: MTLRenderPipelineState

    init(device: MTLDevice, pixelFormat: MTLPixelFormat) throws {
        self.device = device
        guard let q = device.makeCommandQueue() else {
            throw NSError(domain: "TradeChartEngine.MetalRenderer", code: -1,
                          userInfo: [NSLocalizedDescriptionKey: "command queue 생성 실패"])
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

        self.pipelineTriangles = try device.makeRenderPipelineState(descriptor: desc)
        self.pipelineLines     = try device.makeRenderPipelineState(descriptor: desc)
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
            enc.setRenderPipelineState(prim == .line ? pipelineLines : pipelineTriangles)
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

private let shaderSource = """
#include <metal_stdlib>
using namespace metal;

struct VIn  { float2 pos [[attribute(0)]]; float4 col [[attribute(1)]]; };
struct VOut { float4 pos [[position]];     float4 col;                };

vertex VOut tce_vs(VIn in [[stage_in]],
                   constant float2& size [[buffer(1)]]) {
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
#endif
