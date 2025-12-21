// swift-tools-version: 6.1

import PackageDescription

let package = Package(
    name: "Tui",
    products: [
        .library(name: "Tui", targets: ["Tui"]),
        .executable(name: "TuiInterpreter", targets: ["TuiInterpreter"])
    ],
    targets: [
        .target(
            name: "Tui",
            dependencies: ["glm"],
            path: "source",
            publicHeadersPath: "./",
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        ),
        .target(
            name: "glm",
            dependencies: ["tui_simd"],
            path: "thirdParty/glm/glm",
            exclude: ["tui_simd/"],
            publicHeadersPath: "./",
            cxxSettings: [
                .headerSearchPath("../"),
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        ),
        .target(
            name: "tui_simd",
            path: "thirdParty/glm/glm/tui_simd",
            publicHeadersPath: "./",
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        ),
        .executableTarget(
            name: "TuiInterpreter",
            dependencies: ["Tui"],
            path: "interpreter/",
            exclude: ["linux/", "windows/", "macos/swift"],
            publicHeadersPath: "./",
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        )
    ],
    cxxLanguageStandard: .cxx11
)
