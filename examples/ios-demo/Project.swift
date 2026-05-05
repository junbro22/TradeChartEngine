import ProjectDescription

let project = Project(
    name: "TradeChartDemo",
    organizationName: "TradeChart",
    targets: [
        .target(
            name: "TradeChartDemo",
            destinations: .iOS,
            product: .app,
            bundleId: "com.example.tradechart.demo",
            deploymentTargets: .iOS("15.0"),
            infoPlist: .extendingDefault(with: [
                "CFBundleDisplayName": "TradeChart Demo",
                "UILaunchScreen": [:],
                "UISupportedInterfaceOrientations": ["UIInterfaceOrientationPortrait"],
            ]),
            sources: ["Sources/**"],
            resources: ["Resources/**"],
            dependencies: [
                .external(name: "TradeChartEngine"),
            ]
        ),
    ]
)
