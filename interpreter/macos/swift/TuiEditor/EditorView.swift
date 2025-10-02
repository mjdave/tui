//
//  ContentView.swift
//  TuiEditor
//
//  A simple tui editor
//
/// Far from being a complete IDE, this is just a simple example of including Tui in a swift project.

import SwiftUI
import Tui

struct EditorView: View {
    
    @Bindable var viewModel: EditorViewModel
    
    init(viewModel: EditorViewModel) {
        self.viewModel = viewModel
    }
    
    var body: some View {
        VStack {
            HStack {
                VStack(alignment: .leading) {
                    Text("Script:")
                        .font(.headline)
                    Spacer()
                    TextEditor(text: $viewModel.attributedScriptText, selection: $viewModel.selection)
                        .font(.system(size: viewModel.fontSize))
                        .onKeyPress { key in
                            viewModel.keyPressed(key)
                        }
                }
                .frame(maxWidth: .infinity)
                VStack(alignment: .leading) {
                    HStack {
                        Text("Output:")
                            .font(.headline)
                        Spacer()
                    }
                    ScrollView {
                        VStack(alignment: .leading) {
                            Text(viewModel.outputText)
                                .font(.system(size: viewModel.fontSize))
                                .textSelection(.enabled)
                                .multilineTextAlignment(.leading)
                                .frame(maxWidth: .infinity, alignment: .leading)
                        }
                        .frame(maxWidth: .infinity)
                    }
                    .defaultScrollAnchor(.bottom)
                    .frame(maxWidth: .infinity)
                    .background(.background.tertiary)
                }
                .frame(maxWidth: .infinity)
            }
            Button {
                viewModel.runTui()
            } label: {
                Text("Run Tui ⌘ R")
            }
            
        }
        .padding()
        .onChange(of: viewModel.attributedScriptText) { _, newValue in
            viewModel.clearLog()
        }
    }
}

extension NSTextView {
    open override var frame: CGRect {
        didSet {
            self.isAutomaticQuoteSubstitutionEnabled = false
        }
    }
}

#Preview {
    EditorView(viewModel: EditorViewModel())
}
