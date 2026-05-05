// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "TradeChartEngine",
    platforms: [.iOS(.v15)],
    products: [
        .library(name: "TradeChartEngine", targets: ["TradeChartEngine"]),
    ],
    targets: [
        // C ABI를 묶은 binary target
        .binaryTarget(
            name: "TradeChartEngineC",
            path: "TradeChartEngine.xcframework"
        ),
        // Swift 친화적 wrapper
        .target(
            name: "TradeChartEngine",
            dependencies: ["TradeChartEngineC"],
            path: "Sources/TradeChartEngine"
        ),
    ]
)
