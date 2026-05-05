#if os(iOS)
import SwiftUI
import MetalKit

/// SwiftUI 진입점. MTKView 위에 텍스트 라벨 overlay.
public struct TradeChartView: View {
    public let chart: Chart
    public var onCrosshairChange: ((Chart.CrosshairInfo) -> Void)?

    @State private var labels: [ChartLabel] = []
    @State private var pixelSize: CGSize = .zero

    public init(chart: Chart,
                onCrosshairChange: ((Chart.CrosshairInfo) -> Void)? = nil) {
        self.chart = chart
        self.onCrosshairChange = onCrosshairChange
    }

    public var body: some View {
        ZStack {
            ChartMetalLayer(
                chart: chart,
                onCrosshairChange: onCrosshairChange,
                labels: $labels,
                pixelSize: $pixelSize
            )

            // 텍스트 overlay — MTKView가 그린 그리드선 위에 글자
            GeometryReader { geo in
                let scale: CGFloat = pixelSize.width > 0
                    ? geo.size.width / pixelSize.width
                    : 1.0
                ForEach(labels.indices, id: \.self) { i in
                    labelView(labels[i], scale: scale)
                }
            }
            .allowsHitTesting(false)   // 제스처가 MTKView로 통과
        }
    }

    @ViewBuilder
    private func labelView(_ label: ChartLabel, scale: CGFloat) -> some View {
        let textColor = Color(red: Double(label.color.r),
                              green: Double(label.color.g),
                              blue:  Double(label.color.b),
                              opacity: Double(label.color.a))
        let bgColor = Color(red: Double(label.background.r),
                            green: Double(label.background.g),
                            blue:  Double(label.background.b),
                            opacity: Double(label.background.a))

        Text(label.text)
            .font(.system(size: 10, weight: label.kind == .lastPrice ? .semibold : .regular).monospacedDigit())
            .foregroundStyle(textColor)
            .padding(.horizontal, label.background.a > 0 ? 4 : 0)
            .padding(.vertical,   label.background.a > 0 ? 1 : 0)
            .background(bgColor)
            .cornerRadius(label.background.a > 0 ? 3 : 0)
            .position(
                anchorPosition(label: label, scale: scale)
            )
    }

    private func anchorPosition(label: ChartLabel, scale: CGFloat) -> CGPoint {
        // label.x/y는 pixel 좌표. SwiftUI는 point.
        let px = label.x * scale
        let py = label.y * scale
        // SwiftUI .position은 view의 center 기준. anchor 따라 보정.
        // 텍스트 폭/높이 정확히 모르므로 anchor에 따라 적당한 offset 적용.
        switch label.anchor {
        case .leftCenter:   return CGPoint(x: px + 20, y: py)
        case .rightCenter:  return CGPoint(x: px - 20, y: py)
        case .centerTop:    return CGPoint(x: px,      y: py + 8)
        case .centerBottom: return CGPoint(x: px,      y: py - 8)
        case .centerCenter: return CGPoint(x: px,      y: py)
        }
    }
}

// MARK: - Metal layer (UIViewRepresentable)

private struct ChartMetalLayer: UIViewRepresentable {
    let chart: Chart
    var onCrosshairChange: ((Chart.CrosshairInfo) -> Void)?
    @Binding var labels: [ChartLabel]
    @Binding var pixelSize: CGSize

    func makeCoordinator() -> Coordinator {
        Coordinator(chart: chart,
                    onCrosshairChange: onCrosshairChange,
                    setLabels: { labels = $0 },
                    setPixelSize: { pixelSize = $0 })
    }

    func makeUIView(context: Context) -> MTKView {
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

    func updateUIView(_ uiView: MTKView, context: Context) {
        context.coordinator.onCrosshairChange = onCrosshairChange
        uiView.setNeedsDisplay(uiView.bounds)
    }

    final class Coordinator: NSObject, MTKViewDelegate {
        let chart: Chart
        var onCrosshairChange: ((Chart.CrosshairInfo) -> Void)?
        let setLabels: ([ChartLabel]) -> Void
        let setPixelSize: (CGSize) -> Void
        private var renderer: MetalRenderer?

        init(chart: Chart,
             onCrosshairChange: ((Chart.CrosshairInfo) -> Void)?,
             setLabels: @escaping ([ChartLabel]) -> Void,
             setPixelSize: @escaping (CGSize) -> Void) {
            self.chart = chart
            self.onCrosshairChange = onCrosshairChange
            self.setLabels = setLabels
            self.setPixelSize = setPixelSize
        }

        func attach(view: MTKView) {
            guard let device = view.device else { return }
            renderer = try? MetalRenderer(device: device, pixelFormat: view.colorPixelFormat)
        }

        func mtkView(_ view: MTKView, drawableSizeWillChange size: CGSize) {
            chart.setSize(width: size.width, height: size.height)
            setPixelSize(size)
        }

        func draw(in view: MTKView) {
            guard let renderer,
                  let drawable = view.currentDrawable,
                  let pass = view.currentRenderPassDescriptor else { return }
            chart.setSize(width: view.drawableSize.width, height: view.drawableSize.height)
            let meshes = chart.buildFrame()
            let labels = chart.buildLabels()
            renderer.render(meshes: meshes,
                            drawable: drawable,
                            descriptor: pass,
                            viewportSize: view.drawableSize)
            DispatchQueue.main.async { [weak self] in
                self?.setLabels(labels)
                self?.setPixelSize(view.drawableSize)
            }
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
