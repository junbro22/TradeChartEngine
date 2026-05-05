import SwiftUI
import TradeChartEngine

struct ContentView: View {

    @State private var chart: Chart = {
        let c = Chart()
        let candles = DemoData.generate(count: 200)
        c.setHistory(candles)
        c.visibleCount = 80
        c.setColorScheme(.korea)
        c.setSeriesType(.candle)
        c.setVolumePanelVisible(true)
        c.addIndicator(.sma, period: 20, color: ChartColor(r: 1.00, g: 0.85, b: 0.20))
        c.addIndicator(.ema, period: 60, color: ChartColor(r: 0.45, g: 0.80, b: 1.00))

        // 데모 — 매수 50번째, 매도 130번째 캔들에 마커
        if candles.count > 130 {
            c.addTradeMarker(timestamp: candles[50].timestamp,
                             price: candles[50].low, isBuy: true, quantity: 1500)
            c.addTradeMarker(timestamp: candles[130].timestamp,
                             price: candles[130].high, isBuy: false, quantity: 800)
        }
        return c
    }()

    @State private var showVolume     = true
    @State private var showRSI        = false
    @State private var showMACD       = false
    @State private var showBB         = false
    @State private var showStoch      = false
    @State private var showATR        = false
    @State private var showIchimoku   = false
    @State private var showPSAR       = false
    @State private var showSuperTrend = false
    @State private var showVWAP       = false
    @State private var showDMI        = false
    @State private var showCCI        = false
    @State private var showWilliamsR  = false
    @State private var showOBV        = false
    @State private var showMFI        = false
    @State private var showPivot      = false
    @State private var seriesType: SeriesType = .candle
    @State private var drawingMode: DrawingKind? = nil

    var body: some View {
        VStack(spacing: 0) {
            header
            TradeChartView(chart: chart, drawingMode: $drawingMode)
                .background(Color(red: 0.04, green: 0.05, blue: 0.07))
            controls
        }
        .ignoresSafeArea(edges: .bottom)
    }

    private var header: some View {
        HStack {
            Text("TradeChart Demo")
                .font(.headline)
            Spacer()
            Text("v\(Chart.version)")
                .font(.caption.monospaced())
                .foregroundStyle(.secondary)
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 12)
        .background(Color(red: 0.08, green: 0.10, blue: 0.13))
        .foregroundStyle(.white)
    }

    private var controls: some View {
        VStack(spacing: 8) {
            HStack {
                Picker("종류", selection: $seriesType) {
                    Text("캔들").tag(SeriesType.candle)
                    Text("라인").tag(SeriesType.line)
                }
                .pickerStyle(.segmented)
                .onChange(of: seriesType) { v in chart.setSeriesType(v) }

                Toggle("거래량", isOn: $showVolume)
                    .onChange(of: showVolume) { v in chart.setVolumePanelVisible(v) }
                    .toggleStyle(.switch)
                    .labelsHidden()
                    .frame(width: 50)
            }

            ScrollView(.horizontal, showsIndicators: false) {
                HStack(spacing: 8) {
                    indicatorChip("BB",    isOn: $showBB)    { on in
                        on ? chart.addBollinger(period: 20, stddev: 2.0,
                                                 color: ChartColor(r: 0.55, g: 0.85, b: 1.0))
                           : chart.removeIndicator(.bollinger, period: 20)
                    }
                    indicatorChip("RSI",   isOn: $showRSI)   { on in
                        on ? chart.addRSI(period: 14, color: ChartColor(r: 0.95, g: 0.55, b: 0.95))
                           : chart.removeIndicator(.rsi, period: 14)
                    }
                    indicatorChip("MACD",  isOn: $showMACD)  { on in
                        on ? chart.addMACD(
                                fast: 12, slow: 26, signal: 9,
                                lineColor:   ChartColor(r: 0.30, g: 0.85, b: 1.00),
                                signalColor: ChartColor(r: 1.00, g: 0.65, b: 0.20),
                                histColor:   ChartColor(r: 0.50, g: 0.50, b: 0.50, a: 0.5)
                              )
                           : chart.removeIndicator(.macd, period: 12)
                    }
                    indicatorChip("Stoch", isOn: $showStoch) { on in
                        on ? chart.addStochastic(
                                kPeriod: 14, dPeriod: 3, smooth: 3,
                                kColor: ChartColor(r: 1.0, g: 0.85, b: 0.30),
                                dColor: ChartColor(r: 0.50, g: 0.80, b: 1.0)
                              )
                           : chart.removeIndicator(.stochastic, period: 14)
                    }
                    indicatorChip("ATR",   isOn: $showATR)   { on in
                        on ? chart.addATR(period: 14, color: ChartColor(r: 0.85, g: 0.85, b: 0.55))
                           : chart.removeIndicator(.atr, period: 14)
                    }
                    indicatorChip("Ichimoku", isOn: $showIchimoku) { on in
                        on ? chart.addIchimoku(
                                tenkanColor: ChartColor(r: 0.30, g: 0.85, b: 1.00),
                                kijunColor:  ChartColor(r: 1.00, g: 0.55, b: 0.30))
                           : chart.removeIndicator(.ichimoku, period: 9)
                    }
                    indicatorChip("PSAR",  isOn: $showPSAR) { on in
                        on ? chart.addPSAR(color: ChartColor(r: 1.00, g: 0.95, b: 0.30))
                           : chart.removeIndicator(.psar, period: 0)
                    }
                    indicatorChip("ST",    isOn: $showSuperTrend) { on in
                        on ? chart.addSuperTrend(color: ChartColor(r: 0.50, g: 1.00, b: 0.50))
                           : chart.removeIndicator(.supertrend, period: 10)
                    }
                    indicatorChip("VWAP",  isOn: $showVWAP) { on in
                        on ? chart.addVWAP(color: ChartColor(r: 0.95, g: 0.55, b: 0.95))
                           : chart.removeIndicator(.vwap, period: 0)
                    }
                    indicatorChip("DMI",   isOn: $showDMI) { on in
                        on ? chart.addDMI(period: 14,
                                          plusDIColor:  ChartColor(r: 0.30, g: 0.85, b: 0.45),
                                          minusDIColor: ChartColor(r: 0.95, g: 0.30, b: 0.30),
                                          adxColor:     ChartColor(r: 1.00, g: 0.85, b: 0.30))
                           : chart.removeIndicator(.dmiAdx, period: 14)
                    }
                    indicatorChip("CCI",   isOn: $showCCI) { on in
                        on ? chart.addCCI(period: 20, color: ChartColor(r: 0.50, g: 0.85, b: 1.00))
                           : chart.removeIndicator(.cci, period: 20)
                    }
                    indicatorChip("W%R",   isOn: $showWilliamsR) { on in
                        on ? chart.addWilliamsR(period: 14, color: ChartColor(r: 1.00, g: 0.55, b: 0.95))
                           : chart.removeIndicator(.williamsR, period: 14)
                    }
                    indicatorChip("OBV",   isOn: $showOBV) { on in
                        on ? chart.addOBV(color: ChartColor(r: 0.85, g: 0.85, b: 0.55))
                           : chart.removeIndicator(.obv, period: 0)
                    }
                    indicatorChip("MFI",   isOn: $showMFI) { on in
                        on ? chart.addMFI(period: 14, color: ChartColor(r: 0.45, g: 0.95, b: 0.85))
                           : chart.removeIndicator(.mfi, period: 14)
                    }
                    indicatorChip("Pivot", isOn: $showPivot) { on in
                        on ? chart.addPivotStandard(
                                pColor:  ChartColor(r: 1.00, g: 0.85, b: 0.30),
                                rsColor: ChartColor(r: 0.85, g: 0.85, b: 0.85, a: 0.6))
                           : chart.removeIndicator(.pivotStandard, period: 0)
                    }
                }
            }

            // 드로잉 도구 툴바
            HStack(spacing: 6) {
                drawingChip("✕",  active: drawingMode == nil)        { drawingMode = nil }
                drawingChip("―",  active: drawingMode == .horizontal){ drawingMode = .horizontal }
                drawingChip("│",  active: drawingMode == .vertical)  { drawingMode = .vertical }
                drawingChip("╱",  active: drawingMode == .trendline) { drawingMode = .trendline }
                drawingChip("▭",  active: drawingMode == .rectangle) { drawingMode = .rectangle }
                drawingChip("Fib", active: drawingMode == .fibRetracement) { drawingMode = .fibRetracement }
                drawingChip("📏", active: drawingMode == .measure)   { drawingMode = .measure }
                Spacer()
                Button("Clear") {
                    chart.clearDrawings()
                }
                .font(.caption.monospaced())
                .foregroundStyle(.red)
            }
        }
        .padding(.horizontal, 16)
        .padding(.vertical, 10)
        .background(Color(red: 0.08, green: 0.10, blue: 0.13))
        .foregroundStyle(.white)
    }

    private func drawingChip(_ label: String, active: Bool, action: @escaping () -> Void) -> some View {
        Button(action: action) {
            Text(label)
                .font(.caption.monospaced())
                .foregroundStyle(active ? .black : .white)
                .padding(.horizontal, 9)
                .padding(.vertical, 5)
                .background(active ? Color.cyan.opacity(0.85) : Color.white.opacity(0.10), in: Capsule())
        }
    }

    private func indicatorChip(_ label: String,
                               isOn: Binding<Bool>,
                               apply: @escaping (Bool) -> Void) -> some View {
        Button {
            isOn.wrappedValue.toggle()
            apply(isOn.wrappedValue)
        } label: {
            Text(label)
                .font(.caption.monospaced())
                .foregroundStyle(isOn.wrappedValue ? .black : .white)
                .padding(.horizontal, 10)
                .padding(.vertical, 6)
                .background(
                    isOn.wrappedValue ? Color.yellow.opacity(0.85) : Color.white.opacity(0.10),
                    in: Capsule()
                )
        }
    }
}
