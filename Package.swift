// swift-tools-version: 6.1

import PackageDescription

let package = Package(
    name: "Tui",
    products: [
        .library(name: "Tui", targets: ["Tui"]),
        .executable(name: "tui-interpreter", targets: ["tui-interpreter"])
    ],
    targets: [
        .target(
            name: "Tui",
            dependencies: ["tui-glm"],
            path: "source",
            publicHeadersPath: "./"
        ),
        .target(
            name: "tui-glm",
            path: "thirdParty/glm/glm",
            publicHeadersPath: "../glm",
            cxxSettings: [
                .headerSearchPath("../")
            ]
        ),
        .executableTarget(
            name: "tui-interpreter",
            dependencies: ["Tui"],
            path: "interpreter/",
            exclude: ["linux/", "windows/"],
            publicHeadersPath: "./"
        )
    ],
    cxxLanguageStandard: .cxx11
)
