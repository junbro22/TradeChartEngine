import Foundation
import TradeChartEngineC

// MARK: - 공개 타입

public struct Candle: Sendable, Equatable {
    public let timestamp: TimeInterval
    public let open: Double
    public let high: Double
    public let low: Double
    public let close: Double
    public let volume: Double

    public init(timestamp: TimeInterval, open: Double, high: Double, low: Double, close: Double, volume: Double) {
        self.timestamp = timestamp
        self.open = open
        self.high = high
        self.low = low
        self.close = close
        self.volume = volume
    }

    fileprivate var c: TceCandle {
        TceCandle(timestamp: timestamp, open: open, high: high, low: low, close: close, volume: volume)
    }
}

public enum SeriesType: Int32, Sendable {
    case candle = 0
    case line   = 1
    case area   = 2
}

public enum ColorScheme: Int32, Sendable {
    case korea = 0   // 양봉 빨강 / 음봉 파랑
    case us    = 1   // 양봉 초록 / 음봉 빨강
}

public enum IndicatorKind: Int32, Sendable {
    // Overlay
    case sma        = 0
    case ema        = 1
    case bollinger  = 2
    case ichimoku   = 3
    case psar       = 4
    case supertrend = 5
    case vwap       = 6
    case pivotStandard  = 7
    case pivotFibonacci = 8
    case pivotCamarilla = 9
    // Subpanel
    case rsi        = 100
    case macd       = 101
    case stochastic = 102
    case atr        = 103
    case dmiAdx     = 104
    case cci        = 105
    case williamsR  = 106
    case obv        = 107
    case mfi        = 108
}

public struct ChartColor: Sendable {
    public var r: Float
    public var g: Float
    public var b: Float
    public var a: Float
    public init(r: Float, g: Float, b: Float, a: Float = 1.0) {
        self.r = r; self.g = g; self.b = b; self.a = a
    }
    fileprivate var c: TceColor { TceColor(r: r, g: g, b: b, a: a) }
}

public struct ChartVertex: Sendable {
    public let x: Float
    public let y: Float
    public let r: Float
    public let g: Float
    public let b: Float
    public let a: Float
}

public enum Primitive: Int32, Sendable {
    case triangles = 0
    case lines     = 1
}

public struct Mesh {
    public let vertices: [ChartVertex]
    public let indices: [UInt32]
    public let primitive: Primitive
}

// MARK: - Labels

public enum LabelKind: Int32, Sendable {
    case priceAxis        = 0
    case timeAxis         = 1
    case lastPrice        = 2
    case crosshairPrice   = 3
    case crosshairTime    = 4
}

public enum TextAnchor: Int32, Sendable {
    case leftCenter   = 0
    case rightCenter  = 1
    case centerTop    = 2
    case centerBottom = 3
    case centerCenter = 4
}

public struct ChartLabel: Sendable {
    public let text: String
    public let x: CGFloat
    public let y: CGFloat
    public let anchor: TextAnchor
    public let kind: LabelKind
    public let color: ChartColor
    public let background: ChartColor   // alpha 0이면 배경 없음
}

// MARK: - Chart

public final class Chart {
    private let ctx: OpaquePointer

    public init() {
        guard let p = tce_create() else { fatalError("tce_create failed") }
        self.ctx = p
    }

    deinit {
        tce_destroy(ctx)
    }

    public static var version: String {
        String(cString: tce_version())
    }

    // MARK: 데이터

    public func setHistory(_ candles: [Candle]) {
        let raw = candles.map(\.c)
        raw.withUnsafeBufferPointer {
            tce_set_history(ctx, $0.baseAddress, $0.count)
        }
    }

    public func appendCandle(_ candle: Candle) {
        var c = candle.c
        tce_append_candle(ctx, &c)
    }

    public func updateLast(close: Double, volume: Double) {
        tce_update_last(ctx, close, volume)
    }

    public var candleCount: Int {
        tce_candle_count(ctx)
    }

    // MARK: 설정

    public func setSize(width: CGFloat, height: CGFloat) {
        tce_set_size(ctx, Float(width), Float(height))
    }

    public func setSeriesType(_ type: SeriesType) {
        tce_set_series_type(ctx, TceSeriesType(rawValue: UInt32(type.rawValue)))
    }

    public func setColorScheme(_ scheme: ColorScheme) {
        tce_set_color_scheme(ctx, TceColorScheme(rawValue: UInt32(scheme.rawValue)))
    }

    public func setVolumePanelVisible(_ visible: Bool) {
        tce_set_volume_panel_visible(ctx, visible ? 1 : 0)
    }

    // MARK: 지표

    public func addIndicator(_ kind: IndicatorKind, period: Int, color: ChartColor) {
        tce_add_indicator(
            ctx,
            TceIndicatorKind(rawValue: UInt32(kind.rawValue)),
            Int32(period),
            color.c
        )
    }

    public func removeIndicator(_ kind: IndicatorKind, period: Int) {
        tce_remove_indicator(
            ctx,
            TceIndicatorKind(rawValue: UInt32(kind.rawValue)),
            Int32(period)
        )
    }

    public func clearIndicators() {
        tce_clear_indicators(ctx)
    }

    public func addBollinger(period: Int = 20, stddev: Double = 2.0, color: ChartColor) {
        tce_add_bollinger(ctx, Int32(period), stddev, color.c)
    }

    public func addRSI(period: Int = 14, color: ChartColor) {
        tce_add_rsi(ctx, Int32(period), color.c)
    }

    public func addMACD(fast: Int = 12, slow: Int = 26, signal: Int = 9,
                        lineColor: ChartColor, signalColor: ChartColor, histColor: ChartColor) {
        tce_add_macd(ctx, Int32(fast), Int32(slow), Int32(signal),
                     lineColor.c, signalColor.c, histColor.c)
    }

    public func addStochastic(kPeriod: Int = 14, dPeriod: Int = 3, smooth: Int = 3,
                              kColor: ChartColor, dColor: ChartColor) {
        tce_add_stochastic(ctx, Int32(kPeriod), Int32(dPeriod), Int32(smooth),
                           kColor.c, dColor.c)
    }

    public func addATR(period: Int = 14, color: ChartColor) {
        tce_add_atr(ctx, Int32(period), color.c)
    }

    public func addIchimoku(tenkan: Int = 9, kijun: Int = 26,
                            senkouB: Int = 52, displacement: Int = 26,
                            tenkanColor: ChartColor, kijunColor: ChartColor) {
        tce_add_ichimoku(ctx, Int32(tenkan), Int32(kijun), Int32(senkouB), Int32(displacement),
                         tenkanColor.c, kijunColor.c)
    }

    public func addPSAR(step: Double = 0.02, max: Double = 0.2, color: ChartColor) {
        tce_add_psar(ctx, step, max, color.c)
    }

    public func addSuperTrend(period: Int = 10, multiplier: Double = 3.0, color: ChartColor) {
        tce_add_supertrend(ctx, Int32(period), multiplier, color.c)
    }

    public func addVWAP(color: ChartColor) {
        tce_add_vwap(ctx, color.c)
    }

    public func addDMI(period: Int = 14,
                       plusDIColor: ChartColor, minusDIColor: ChartColor, adxColor: ChartColor) {
        tce_add_dmi(ctx, Int32(period), plusDIColor.c, minusDIColor.c, adxColor.c)
    }

    public func addCCI(period: Int = 20, color: ChartColor) {
        tce_add_cci(ctx, Int32(period), color.c)
    }

    public func addWilliamsR(period: Int = 14, color: ChartColor) {
        tce_add_williams_r(ctx, Int32(period), color.c)
    }

    public func addOBV(color: ChartColor) {
        tce_add_obv(ctx, color.c)
    }

    public func addMFI(period: Int = 14, color: ChartColor) {
        tce_add_mfi(ctx, Int32(period), color.c)
    }

    // MARK: 뷰포트

    public var visibleCount: Int {
        get { Int(tce_visible_count(ctx)) }
        set { tce_set_visible_count(ctx, Int32(newValue)) }
    }

    public var rightOffset: Int {
        get { Int(tce_right_offset(ctx)) }
        set { tce_set_right_offset(ctx, Int32(newValue)) }
    }

    public func pan(deltaPixels: CGFloat) {
        tce_pan(ctx, Float(deltaPixels))
    }

    public func zoom(factor: CGFloat, anchorX: CGFloat) {
        tce_zoom(ctx, Float(factor), Float(anchorX))
    }

    /// Wrapper는 raw 픽셀만 dispatch — 의미 해석은 엔진이.
    public func applyPinch(scale: CGFloat, anchorPx: CGFloat) {
        tce_apply_pinch(ctx, Float(scale), Float(anchorPx))
    }

    public func applyPan(dxPx: CGFloat) {
        tce_apply_pan(ctx, Float(dxPx))
    }

    // MARK: 크로스헤어

    public func setCrosshair(x: CGFloat, y: CGFloat) {
        tce_set_crosshair(ctx, Float(x), Float(y))
    }

    public func clearCrosshair() {
        tce_clear_crosshair(ctx)
    }

    public struct CrosshairInfo: Sendable {
        public let visible: Bool
        public let candleIndex: Int
        public let price: Double
        public let timestamp: TimeInterval
        public let screenX: Float
        public let screenY: Float
    }

    public var crosshair: CrosshairInfo {
        let info = tce_crosshair_info(ctx)
        return CrosshairInfo(
            visible: info.visible != 0,
            candleIndex: Int(info.candle_index),
            price: info.price,
            timestamp: info.timestamp,
            screenX: info.screen_x,
            screenY: info.screen_y
        )
    }

    // MARK: 렌더

    public var autoScroll: Bool {
        get { tce_auto_scroll(ctx) != 0 }
        set { tce_set_auto_scroll(ctx, newValue ? 1 : 0) }
    }

    /// buildFrame 직후 호출 — 같은 layout으로 라벨 좌표 계산.
    public func buildLabels() -> [ChartLabel] {
        let raw = tce_build_labels(ctx)
        var out: [ChartLabel] = []
        out.reserveCapacity(raw.count)
        for i in 0..<raw.count {
            let l = raw.items[i]
            let text = l.text.flatMap { String(cString: $0) } ?? ""
            out.append(ChartLabel(
                text: text,
                x: CGFloat(l.x),
                y: CGFloat(l.y),
                anchor: TextAnchor(rawValue: Int32(l.anchor.rawValue)) ?? .leftCenter,
                kind: LabelKind(rawValue: Int32(l.kind.rawValue)) ?? .priceAxis,
                color: ChartColor(r: l.color.r, g: l.color.g, b: l.color.b, a: l.color.a),
                background: ChartColor(r: l.background.r, g: l.background.g, b: l.background.b, a: l.background.a)
            ))
        }
        return out
    }

    /// 현재 상태로 한 프레임의 메시 묶음을 빌드.
    /// 반환된 Mesh는 Swift가 소유 (Vertex/Index는 복사됨).
    public func buildFrame() -> [Mesh] {
        let frame = tce_build_frame(ctx)
        var result: [Mesh] = []
        result.reserveCapacity(frame.mesh_count)
        for i in 0..<frame.mesh_count {
            let m = frame.meshes[i]
            var verts: [ChartVertex] = []
            verts.reserveCapacity(m.vertex_count)
            for vi in 0..<m.vertex_count {
                let v = m.vertices[vi]
                verts.append(ChartVertex(x: v.x, y: v.y, r: v.r, g: v.g, b: v.b, a: v.a))
            }
            var indices: [UInt32] = []
            indices.reserveCapacity(m.index_count)
            for ii in 0..<m.index_count {
                indices.append(m.indices[ii])
            }
            result.append(Mesh(
                vertices: verts,
                indices: indices,
                primitive: Primitive(rawValue: m.primitive) ?? .triangles
            ))
        }
        tce_release_frame(frame)
        return result
    }
}
