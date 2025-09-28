// swift-tools-version: 6.1
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "tui",
    products: [
        // Products define the executables and libraries a package produces, making them visible to other packages.
        // .library(
        //     name: "tui",
        //     targets: ["tui"]),
        .executable(name: "tui-interpreter", targets: ["tui-interpreter"])
    ],
    targets: [
        // Targets are the basic building blocks of a package, defining a module or a test suite.
        // Targets can depend on other targets in this package and products from dependencies.
        .target(
            name: "tui",
            path: "source"
        ),
        .target(
            name: "glm",
            path: "thirdParty/glm",
            publicHeadersPath: "thirdParty/glm"
        ),
        .executableTarget(
            name: "tui-interpreter",
            dependencies: ["glm"],
            path: "interpreter/macos/tui"
        )
    ],
    cxxLanguageStandard: .cxx11
)
