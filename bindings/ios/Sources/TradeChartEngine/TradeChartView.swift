#if os(iOS)
import SwiftUI
import MetalKit

/// SwiftUI 진입점. 호스트 앱은 이 뷰에 Chart 인스턴스만 전달하면 된다.
///
/// 기본 제공:
/// - 핀치 줌 / 드래그 팬 / 길게 누르기 크로스헤어
/// - Metal 렌더 파이프라인 (60fps)
/// - 레이아웃 변경 자동 반영
///
/// 호스트 앱은 데이터/지표/색 스킴 등을 Chart 객체에 설정하기만 하면 됨.
public struct TradeChartView: UIViewRepresentable {

    public let chart: Chart
    public var onCrosshairChange: ((Chart.CrosshairInfo) -> Void)?

    public init(chart: Chart,
                onCrosshairChange: ((Chart.CrosshairInfo) -> Void)? = nil) {
        self.chart = chart
        self.onCrosshairChange = onCrosshairChange
    }

    public func makeCoordinator() -> Coordinator {
        Coordinator(chart: chart, onCrosshairChange: onCrosshairChange)
    }

    public func makeUIView(context: Context) -> MTKView {
        let v = MTKView(frame: .zero, device: MTLCreateSystemDefaultDevice())
        v.colorPixelFormat = .bgra8Unorm
        v.clearColor = MTLClearColorMake(0.07, 0.08, 0.11, 1.0)
        v.preferredFramesPerSecond = 60
        v.delegate = context.coordinator
        context.coordinator.attach(view: v)

        let pinch = UIPinchGestureRecognizer(target: context.coordinator,
                                             action: #selector(Coordinator.onPinch(_:)))
        let pan   = UIPanGestureRecognizer(target: context.coordinator,
                                           action: #selector(Coordinator.onPan(_:)))
        let long  = UILongPressGestureRecognizer(target: context.coordinator,
                                                 action: #selector(Coordinator.onLongPress(_:)))
        long.minimumPressDuration = 0.2
        v.addGestureRecognizer(pinch)
        v.addGestureRecognizer(pan)
        v.addGestureRecognizer(long)
        return v
    }

    public func updateUIView(_ uiView: MTKView, context: Context) {
        context.coordinator.onCrosshairChange = onCrosshairChange
        uiView.setNeedsDisplay(uiView.bounds)
    }

    // MARK: - Coordinator

    public final class Coordinator: NSObject, MTKViewDelegate {
        let chart: Chart
        var onCrosshairChange: ((Chart.CrosshairInfo) -> Void)?
        private var renderer: MetalRenderer?

        init(chart: Chart, onCrosshairChange: ((Chart.CrosshairInfo) -> Void)?) {
            self.chart = chart
            self.onCrosshairChange = onCrosshairChange
        }

        func attach(view: MTKView) {
            guard let device = view.device else { return }
            renderer = try? MetalRenderer(device: device, pixelFormat: view.colorPixelFormat)
        }

        public func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
            chart.setSize(width: size.width, height: size.height)
        }

        public func draw(in view: MTKView) {
            guard let renderer,
                  let drawable = view.currentDrawable,
                  let pass = view.currentRenderPassDescriptor else { return }
            chart.setSize(width: view.drawableSize.width, height: view.drawableSize.height)
            let meshes = chart.buildFrame()
            renderer.render(meshes: meshes,
                            drawable: drawable,
                            descriptor: pass,
                            viewportSize: view.drawableSize)
        }

        @objc func onPinch(_ g: UIPinchGestureRecognizer) {
            guard let view = g.view, g.state == .changed else { return }
            let factor = Float(g.scale)
            if factor > 0 {
                let anchor = g.location(in: view).x * view.contentScaleFactor
                chart.zoom(factor: CGFloat(factor), anchorX: anchor)
            }
            g.scale = 1
            view.setNeedsDisplay()
        }

        @objc func onPan(_ g: UIPanGestureRecognizer) {
            guard let view = g.view, g.state == .changed else { return }
            let translation = g.translation(in: view)
            chart.pan(deltaPixels: -translation.x * view.contentScaleFactor)
            g.setTranslation(.zero, in: view)
            view.setNeedsDisplay()
        }

        @objc func onLongPress(_ g: UILongPressGestureRecognizer) {
            guard let view = g.view else { return }
            switch g.state {
            case .began, .changed:
                let p = g.location(in: view)
                chart.setCrosshair(x: p.x * view.contentScaleFactor,
                                   y: p.y * view.contentScaleFactor)
                onCrosshairChange?(chart.crosshair)
            case .ended, .cancelled, .failed:
                chart.clearCrosshair()
                onCrosshairChange?(chart.crosshair)
            default: break
            }
            view.setNeedsDisplay()
        }
    }
}
#endif
