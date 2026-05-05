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
    case sma = 0
    case ema = 1
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

// MARK: - Chart

public final class Chart {
    private let ctx: OpaquePointer

    public init() {
        guard let p = tce_create() else { fatalError("tce_create failed") }
        self.ctx = OpaquePointer(p)
    }

    deinit {
        tce_destroy(.init(ctx))
    }

    public static var version: String {
        String(cString: tce_version())
    }

    // MARK: 데이터

    public func setHistory(_ candles: [Candle]) {
        let raw = candles.map(\.c)
        raw.withUnsafeBufferPointer {
            tce_set_history(.init(ctx), $0.baseAddress, $0.count)
        }
    }

    public func appendCandle(_ candle: Candle) {
        var c = candle.c
        tce_append_candle(.init(ctx), &c)
    }

    public func updateLast(close: Double, volume: Double) {
        tce_update_last(.init(ctx), close, volume)
    }

    public var candleCount: Int {
        tce_candle_count(.init(ctx))
    }

    // MARK: 설정

    public func setSize(width: CGFloat, height: CGFloat) {
        tce_set_size(.init(ctx), Float(width), Float(height))
    }

    public func setSeriesType(_ type: SeriesType) {
        tce_set_series_type(.init(ctx), TceSeriesType(rawValue: UInt32(type.rawValue)))
    }

    public func setColorScheme(_ scheme: ColorScheme) {
        tce_set_color_scheme(.init(ctx), TceColorScheme(rawValue: UInt32(scheme.rawValue)))
    }

    public func setVolumePanelVisible(_ visible: Bool) {
        tce_set_volume_panel_visible(.init(ctx), visible ? 1 : 0)
    }

    // MARK: 지표

    public func addIndicator(_ kind: IndicatorKind, period: Int, color: ChartColor) {
        tce_add_indicator(
            .init(ctx),
            TceIndicatorKind(rawValue: UInt32(kind.rawValue)),
            Int32(period),
            color.c
        )
    }

    public func removeIndicator(_ kind: IndicatorKind, period: Int) {
        tce_remove_indicator(
            .init(ctx),
            TceIndicatorKind(rawValue: UInt32(kind.rawValue)),
            Int32(period)
        )
    }

    public func clearIndicators() {
        tce_clear_indicators(.init(ctx))
    }

    // MARK: 뷰포트

    public var visibleCount: Int {
        get { Int(tce_visible_count(.init(ctx))) }
        set { tce_set_visible_count(.init(ctx), Int32(newValue)) }
    }

    public var rightOffset: Int {
        get { Int(tce_right_offset(.init(ctx))) }
        set { tce_set_right_offset(.init(ctx), Int32(newValue)) }
    }

    public func pan(deltaPixels: CGFloat) {
        tce_pan(.init(ctx), Float(deltaPixels))
    }

    public func zoom(factor: CGFloat, anchorX: CGFloat) {
        tce_zoom(.init(ctx), Float(factor), Float(anchorX))
    }

    // MARK: 크로스헤어

    public func setCrosshair(x: CGFloat, y: CGFloat) {
        tce_set_crosshair(.init(ctx), Float(x), Float(y))
    }

    public func clearCrosshair() {
        tce_clear_crosshair(.init(ctx))
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
        let info = tce_crosshair_info(.init(ctx))
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

    /// 현재 상태로 한 프레임의 메시 묶음을 빌드.
    /// 반환된 Mesh는 Swift가 소유 (Vertex/Index는 복사됨).
    public func buildFrame() -> [Mesh] {
        let frame = tce_build_frame(.init(ctx))
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
