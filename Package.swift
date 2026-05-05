// swift-tools-version: 5.9
import PackageDescription

// TradeChartEngine — C++ 코어의 iOS xcframework를 SPM binary target으로 노출.
// iOS wrapper와 샘플앱은 별도 repo(TradeChart_iOS)에서 이 패키지를 의존.
let package = Package(
    name: "TradeChartEngine",
    platforms: [.iOS(.v15)],
    products: [
        .library(name: "TradeChartEngineC", targets: ["TradeChartEngineC"]),
    ],
    targets: [
        .binaryTarget(
            name: "TradeChartEngineC",
            path: "TradeChartEngine.xcframework"
        ),
    ]
)
