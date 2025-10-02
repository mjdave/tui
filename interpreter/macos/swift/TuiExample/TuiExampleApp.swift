//
//  TuiExampleApp.swift
//  TuiExample
//

import SwiftUI

import Tui

@main
struct TuiTestingApp: App {
    
    let viewModel = ViewModel()
    
    var body: some Scene {
        WindowGroup {
            ContentView(viewModel: viewModel)
        }
        .commands {
            CommandMenu("Run") {
                Button("Run", systemImage: "play.fill") {
                    viewModel.runTui()
                }
                .keyboardShortcut("R")
            }
        }
    }
}
