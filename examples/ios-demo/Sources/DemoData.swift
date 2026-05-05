import Foundation
import TradeChartEngine

enum DemoData {
    /// 시드 기반 의사 난수 캔들 시계열 — 실제 API 대신 데모용
    static func generate(count: Int = 200, seed: UInt64 = 42) -> [Candle] {
        var rng = SeededRNG(seed: seed)
        var candles: [Candle] = []
        candles.reserveCapacity(count)
        var price: Double = 70_000
        let start = Date(timeIntervalSinceNow: -Double(count) * 86_400)
        for i in 0..<count {
            let drift = Double.random(in: -0.015...0.018, using: &rng)
            let open = price
            let close = max(1, open * (1 + drift))
            let high = max(open, close) * (1 + Double.random(in: 0...0.01, using: &rng))
            let low  = min(open, close) * (1 - Double.random(in: 0...0.01, using: &rng))
            let volume = Double.random(in: 50_000...500_000, using: &rng)
            let ts = start.addingTimeInterval(Double(i) * 86_400).timeIntervalSince1970
            candles.append(Candle(timestamp: ts, open: open, high: high, low: low, close: close, volume: volume))
            price = close
        }
        return candles
    }
}

/// Mersenne-twister 대체 — 단순 LCG. 데모 일관성용.
private struct SeededRNG: RandomNumberGenerator {
    private var state: UInt64
    init(seed: UInt64) { self.state = seed | 1 }
    mutating func next() -> UInt64 {
        state &*= 6_364_136_223_846_793_005
        state &+= 1_442_695_040_888_963_407
        return state
    }
}
