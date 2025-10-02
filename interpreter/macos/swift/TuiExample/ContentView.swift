//
//  ContentView.swift
//  TuiExample
//
//  A simple tui editor
//
/// Far from being a complete IDE, this is just a simple example of including Tui in a swift project.

import SwiftUI
import Tui

struct ContentView: View {
    
    let viewModel: ViewModel
    
    @State var scriptText: String
    
    init(viewModel: ViewModel) {
        self.viewModel = viewModel
        self.scriptText = viewModel.scriptText
    }
        
    var body: some View {
        VStack {
            TextEditor(text: $scriptText)
            Button {
                viewModel.runTui()
            } label: {
                Text("Run Tui ⌘ R")
            }
        }
        .padding()
        .onChange(of: scriptText) { oldValue, newValue in
            viewModel.scriptText = newValue
        }
    }
}

@Observable
class ViewModel {
    
    var scriptText = ""
    
    init() {
        if let scriptPath = Bundle.main.url(forResource: "example", withExtension: "tui")?.path() {
            do {
                scriptText = try String(contentsOfFile: scriptPath, encoding: String.Encoding.utf8)
            } catch {
                debugPrint("Error reading file: \(error)")
            }
        }
    }
    
    func runTui() {
        TuiRef.loadString("\(scriptText)", "debug")
    }
}

#Preview {
    ContentView(viewModel: ViewModel())
}
