//
//  TuiEditorApp.swift
//  TuiEditor
//

import SwiftUI

import Tui
internal import UniformTypeIdentifiers

@main
struct TuiEditorApp: App {
    
    @State private var isImporting: Bool = false
    
    @State var dynamicTypeSize: DynamicTypeSize = .medium
    
    let viewModel = EditorViewModel()
    
    var body: some Scene {
        WindowGroup {
            EditorView(viewModel: viewModel)
                .dynamicTypeSize(dynamicTypeSize)
        }
        .commands {
            CommandMenu("Font") {
                Button("Increase", systemImage: "plus") {
                    viewModel.increaseFontSize()
                }
                .keyboardShortcut("+")
                Button("Decrease", systemImage: "minus") {
                    viewModel.decreaseFontSize()
                }
                .keyboardShortcut("-")
            }
            CommandMenu("Run") {
                Button("Run", systemImage: "play.fill") {
                    viewModel.runTui()
                }
                .keyboardShortcut("R")
            }
            CommandGroup(after: .newItem) {
                Button("Open", systemImage: "arrow.up.forward.square") {
                    isImporting = true
                }
                .keyboardShortcut("O")
                .fileImporter(isPresented: $isImporting, allowedContentTypes: [.init(filenameExtension: "tui")].compactMap{ $0 }) { result in
                    switch result {
                    case .success(let success):
                        viewModel.openFile(success)
                        viewModel.runSyntaxHighlighting()
                    case .failure(let failure):
                        debugPrint("Error opening file: \(failure)")
                    }
                }
                .fileDialogDefaultDirectory(Bundle.main.bundleURL.appending(path: "Contents/Resources"))
                Button("Save", systemImage: "square.and.arrow.down") {
                    viewModel.saveFile()
                }
                .keyboardShortcut("S")
            }
        }
    }
}
