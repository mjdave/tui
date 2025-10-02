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
            dependencies: ["GLM"],
            path: "source",
            publicHeadersPath: "./"
        ),
        .target(
            name: "GLM",
            path: "thirdParty/glm/glm",
            publicHeadersPath: "./",
            cxxSettings: [
                .headerSearchPath("../")
            ]
        ),
        .executableTarget(
            name: "TuiInterpreter",
            dependencies: ["Tui"],
            path: "interpreter/",
            exclude: ["linux/", "windows/"],
            publicHeadersPath: "./",
            swiftSettings: [
                .interoperabilityMode(.Cxx)
            ]
        )
    ],
    cxxLanguageStandard: .cxx11
)
