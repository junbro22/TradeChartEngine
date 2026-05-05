import SwiftUI
import TradeChartEngine

struct ContentView: View {

    @State private var chart: Chart = {
        let c = Chart()
        c.setHistory(DemoData.generate(count: 200))
        c.visibleCount = 80
        c.setColorScheme(.korea)
        c.setSeriesType(.candle)
        c.setVolumePanelVisible(true)
        c.addIndicator(.sma, period: 20, color: ChartColor(r: 1.00, g: 0.85, b: 0.20))
        c.addIndicator(.ema, period: 60, color: ChartColor(r: 0.45, g: 0.80, b: 1.00))
        return c
    }()

    @State private var showVolume: Bool = true
    @State private var seriesType: SeriesType = .candle

    var body: some View {
        VStack(spacing: 0) {
            header

            // 핵심: 한 줄로 차트 사용
            TradeChartView(chart: chart)
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
        .padding(.horizontal, 16)
        .padding(.vertical, 10)
        .background(Color(red: 0.08, green: 0.10, blue: 0.13))
        .foregroundStyle(.white)
    }
}
