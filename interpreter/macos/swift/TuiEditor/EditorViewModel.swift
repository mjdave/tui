//
//  EditorViewModel.swift
//  TuiEditor
//

import Observation
import AppKit
import SwiftUI

import Tui

@Observable
class EditorViewModel {
    
    var scriptText = ""
    var attributedScriptText: AttributedString = ""
    var selection: AttributedTextSelection = .init()
    var outputText = ""
    var inputPipe: Pipe?
    var outputPipe: Pipe?
    
    var fontSize = 12.0
    var customFuncs: [SwiftClosure]
    var argsClosure: SwiftClosure?
        
    let rootTablePointer: UnsafeMutablePointer<TuiTable>?
    
    var fileUrl: URL?
    
    var isEditingSyntaxHighlighting: Bool {
        fileUrl != nil && fileUrl == Bundle.main.url(forResource: "syntaxHighlighting", withExtension: "tui")
    }
    
    init() {
        customFuncs = []
        rootTablePointer = Tui.createRootTable()
        setupCustomFunctions()
        
        fileUrl = Bundle.main.url(forResource: "example", withExtension: "tui")
        
        if let scriptPath = fileUrl?.path() {
            do {
                scriptText = try String(contentsOfFile: scriptPath, encoding: String.Encoding.utf8)
                runSyntaxHighlighting()
            } catch {
                debugPrint("Error reading file: \(error)")
            }
        }
        
        startListeningToStdout()
    }
    
    isolated deinit {
        stopListeningToStdout()
    }
    
    func increaseFontSize() {
        fontSize += 1.0
    }
        
    func decreaseFontSize() {
        fontSize -= 1.0
    }
    
    func runTui() {
        clearLog()
        let returnValueTuiRef = TuiRef.loadString("\(scriptText)", "debug", rootTablePointer)
        
        _ = returnValueTuiRef?.withMemoryRebound(to: TuiTable.self, capacity: 1) { pointer in
            let tuiRef = pointer.pointee.__getUnsafe("returnValue")
            tuiRef?.pointee.retain()
            
            return tuiRef?.withMemoryRebound(to: TuiString.self, capacity: 1) { pointer in
                let stringValue = pointer.pointee.value
                print("Tui completed with returnValue: \(stringValue)")
                return "\(stringValue)"
            }
        }
    }
    
    func runSyntaxHighlighting() {
        var syntaxHighlightingScript: String? = nil
        if let scriptPath = Bundle.main.url(forResource: "syntaxHighlighting", withExtension: "tui")?.path() {
            do {
                syntaxHighlightingScript = try String(contentsOfFile: scriptPath, encoding: String.Encoding.utf8)
            } catch {
                debugPrint("Error reading file: \(error)")
            }
        }
        
        guard let syntaxHighlightingScript else { return }
        
        let returnValueTuiRef = TuiRef.loadString("\(syntaxHighlightingScript)", "syntaxHighlighting", rootTablePointer)
        
        let annotatedScript: String? = returnValueTuiRef?.withMemoryRebound(to: TuiTable.self, capacity: 1) { pointer in
            let tuiRef = pointer.pointee.__getUnsafe("returnValue")
            tuiRef?.pointee.retain()
            
            return tuiRef?.withMemoryRebound(to: TuiString.self, capacity: 1) { pointer in
                let stringValue = pointer.pointee.value
                return "\(stringValue)"
            }
        }
        
        if let annotatedScript {
            attributedScriptText = syntaxHighlightCharLexer(annotatedScript)
        }
    }
    
    private func syntaxHighlightCharLexer(_ annotatedScript: String/*, selection: inout AttributedTextSelection*/) -> AttributedString {
        
        func clearStartOfText() {
            startOfTextIndex = nil
        }
        
        func clearStartOfAttributes() {
            startOfAttributesIndex = nil
        }
        
        let script = AttributedString(annotatedScript)
        var newScript = AttributedString()
        var blueContainer = AttributeContainer()
        blueContainer.foregroundColor = .blue
        var startOfTextIndex: AttributedString.Index?
        var startOfAttributesIndex: AttributedString.Index?
        var word: String?
        
        var currentIndex = script.startIndex
        while currentIndex != script.endIndex {
            let currentChar = script.characters[currentIndex]
            
            if currentChar == "^", script.characters[script.index(beforeCharacter: currentIndex)] != "\"" {
                let nextIndex = script.index(afterCharacter: currentIndex)
                let nextChar = script.characters[nextIndex]
                if nextChar == "[" {
                    currentIndex = script.index(afterCharacter: nextIndex)
                    startOfTextIndex = currentIndex
                }
            } else if let startOfTextIndex, currentChar == "]" {
                word = String(script[startOfTextIndex..<currentIndex].characters)
                clearStartOfText()
            } else if word != nil, currentChar == "(" {
                currentIndex = script.index(afterCharacter: currentIndex)
                startOfAttributesIndex = currentIndex
            } else if let startOfAttributesIndex, currentChar == ")" {
                let attributesString = String(script[startOfAttributesIndex..<currentIndex].characters)
                let strings = attributesString.split(separator: ": ")
                assert(strings.count == 2) // only supporting color now
                let colorString = strings[1]
                if let word {
                    var coloredText: AttributedString?
                    
                    if colorString.hasPrefix("0x") {
                        let hexColor = colorFromHex(String(colorString))
                        var hexContainer = AttributeContainer()
                        hexContainer.foregroundColor = hexColor
                        coloredText = AttributedString(word, attributes: hexContainer)
                    } else {
                        coloredText = AttributedString(word, attributes: blueContainer)
                    }
                    
                    if let coloredText {
                        newScript.append(coloredText)
                    }
                }

                word = nil
                clearStartOfAttributes()
            } else if startOfTextIndex == nil, startOfAttributesIndex == nil {
                newScript.append(AttributedString("\(currentChar)"))
            }
            
            currentIndex = script.index(afterCharacter: currentIndex)
        }
        
        if case .insertionPoint(let index) = selection.indices(in: attributedScriptText) {
            let counter = attributedScriptText.utf16.distance(from: attributedScriptText.startIndex, to: index)
            selection = .init(insertionPoint: newScript.index(newScript.startIndex, offsetByCharacters: counter))
        }
        
        return newScript
    }
    
    func colorFromHex(_ s: String) -> Color? {
        var hex = s.trimmingCharacters(in: .whitespacesAndNewlines)
        if hex.hasPrefix("0x") || hex.hasPrefix("0X") { hex.removeFirst(2) }
        else if hex.hasPrefix("#") { hex.removeFirst() }
        guard hex.count == 6 else { return nil }
        
        let rStart = hex.startIndex
        let rEnd = hex.index(rStart, offsetBy: 2)
        let gEnd = hex.index(rEnd, offsetBy: 2)
        let bEnd = hex.index(gEnd, offsetBy: 2)
        
        let rs = String(hex[rStart..<rEnd])
        let gs = String(hex[rEnd..<gEnd])
        let bs = String(hex[gEnd..<bEnd])
        
        guard let r = UInt8(rs, radix: 16),
              let g = UInt8(gs, radix: 16),
              let b = UInt8(bs, radix: 16) else { return nil }
        
        return Color(red: Double(r)/255.0, green: Double(g)/255.0, blue: Double(b)/255.0)
    }
    
    func clearLog() {
        outputText = ""
    }
    
    @MainActor
    func openFile(_ url: URL) {
        do {
            if url.startAccessingSecurityScopedResource() {
                fileUrl = url
                scriptText = try String(contentsOf: url, encoding: .utf8)
                runSyntaxHighlighting()
                url.stopAccessingSecurityScopedResource()
            } else {
                print("Access to file was not allowed")
            }
        } catch {
            print("Error reading file: \(error)")
        }
    }
    
    func saveFile() {
        do {
            if let url = fileUrl, url.startAccessingSecurityScopedResource() {
                let stringValue = String(attributedScriptText.characters)
                try stringValue.write(to: url, atomically: true, encoding: .utf8)
                url.stopAccessingSecurityScopedResource()
                
                if isEditingSyntaxHighlighting {
                    runSyntaxHighlighting()
                }
            } else {
                print("Access to file was not allowed")
            }
        } catch {
            print("Error writing file: \(error)")
        }
    }
    
    func setupCustomFunctions() {
        guard let rootTablePointer else { return }
        
        var closure = SwiftClosure {
            NSSound.beep()
        }
                
        closure.setFunction("beep", rootTablePointer)
        customFuncs.append(closure)
        
        closure = SwiftClosure {
            sleep(1)
        }
                
        closure.setFunction("sleep", rootTablePointer)
        customFuncs.append(closure)
        
        let currentScriptClosure: (@convention(c) (UnsafeMutableRawPointer) -> UnsafeMutablePointer<CChar>?) = { context in
            let viewModel = Unmanaged<EditorViewModel>.fromOpaque(context).takeUnretainedValue()
            return viewModel.scriptText.withCString { cStr in
                let newString = strdup(cStr) // Caller (C++) frees this
                return newString
            }
        }
        
        let context = Unmanaged.passUnretained(self).toOpaque()
        argsClosure = SwiftClosure(context, currentScriptClosure)
        if let argsClosure {
            argsClosure.setFunctionWithReturnValue("currentScript", rootTablePointer)
            customFuncs.append(closure)
        }
        
        let readInputClosure: (@convention(c) (UnsafeMutableRawPointer) -> UnsafeMutablePointer<CChar>?) = { _ in
            
            let alert = NSAlert()
            alert.addButton(withTitle: "Done")
            alert.messageText = "User input (yes/no): "
            let textView = NSTextView()
            textView.frame.size = CGSize(width: 100, height: 20)
            textView.font = .systemFont(ofSize: 12.0)
            textView.backgroundColor = .textBackgroundColor
            textView.textColor = .labelColor
            alert.accessoryView = textView
            alert.runModal()
            
            let input = textView.string.trimmingCharacters(in: .whitespacesAndNewlines)
            
            return input.withCString { cStr in
                let newString = strdup(cStr) // Caller (C++) frees this
                return newString
            }
        }
        
        argsClosure = SwiftClosure(context, readInputClosure)
        if let argsClosure {
            argsClosure.setFunctionWithReturnValue("readInput", rootTablePointer)
            customFuncs.append(closure)
        }
        
        let bundleLoadClosure: (@convention(c) (UnsafeMutableRawPointer) -> UnsafeMutablePointer<CChar>?) = { args in
            var tuiTable = args.load(as: TuiTable.self)
            let tuiRef = tuiTable.__getArrayUnsafe(0)
            tuiRef?.pointee.retain()
            
            return tuiRef?.withMemoryRebound(to: TuiString.self, capacity: 1) { pointer in
                let filename = "\(pointer.pointee.value)"

                guard let url = Bundle.main.url(forResource: filename, withExtension: "tui") else {
                    print("File not found at bundlePath: \(String(describing: Bundle.main.path(forResource: filename, ofType: "tui")))")
                    return nil
                }
                do {
                    let scriptText = try String(contentsOf: url, encoding: .utf8)
                    let newString = strdup(scriptText)
                    return UnsafeMutablePointer(newString) // Must be freed by caller
                } catch {
                    print("Error reading file: \(error)")
                }
                
                return nil
            }

        }
        
        argsClosure = SwiftClosure(bundleLoadClosure)
        if let argsClosure {
            argsClosure.setFunctionWithArgs("bundleLoad", rootTablePointer)
            customFuncs.append(argsClosure)
        }
        
        setOtherFunctions(rootTablePointer)
        
    }
    
    func scriptModified(_ updatedScript: String) {
        scriptText = updatedScript
        runSyntaxHighlighting()
    }
    
    func keyPressed(_ key: KeyPress) -> KeyPress.Result {
        guard case .insertionPoint(let index) = selection.indices(in: attributedScriptText) else { return .ignored }
        
        switch key.key {
        case .tab:
            let tabSpaces = "    "
            var distance: Int?
            if case .insertionPoint(let index) = selection.indices(in: attributedScriptText) {
                distance = attributedScriptText.utf16.distance(from: attributedScriptText.startIndex, to: index)
            }
            
            attributedScriptText.insert(AttributedString(tabSpaces), at: index)
            scriptModified(String(attributedScriptText.characters))
            
            if let distance {
                selection = .init(insertionPoint: attributedScriptText.index(attributedScriptText.startIndex, offsetByCharacters: distance + tabSpaces.count))
            }
            
            return .handled
        case .return:
            return .ignored
        case .delete:
            attributedScriptText.transform(updating: &selection) { scriptText in
                if case .ranges(let rangeSet) = selection.indices(in: scriptText), let firstRange = rangeSet.ranges.first {
                    scriptText.removeSubrange(firstRange)
                }
            }
            scriptModified(String(attributedScriptText.characters))
            return .ignored
        case .leftArrow, .rightArrow, .upArrow, .downArrow:
            return .ignored
        default:
            if key.characters == "\u{7F}" { // backspace
                attributedScriptText.transform(updating: &selection) { scriptText in
                    let prevIndex = scriptText.index(beforeCharacter: index)
                    scriptText.replaceSubrange(prevIndex..<index, with: AttributedString(""))
                }
                scriptModified(String(attributedScriptText.characters))
                return .handled
            }
        }
        
        var set: CharacterSet = CharacterSet.alphanumerics
        set.formUnion(.punctuationCharacters)
        set.formUnion(.whitespacesAndNewlines)
        set.formUnion(.symbols)
        
        if let unicodeScaler = key.characters.unicodeScalars.first,
           set.contains(unicodeScaler) {
            attributedScriptText.transform(updating: &selection) { scriptText in
                if let char = key.characters.first {
                    scriptText.insert(AttributedString(String(char)), at: index)
                }
            }
            scriptModified(String(attributedScriptText.characters))
        }
        
        return .handled
    }
    
    private func startListeningToStdout() {

        inputPipe = Pipe()
        outputPipe = Pipe()
        
        guard let inputPipe, let outputPipe else { return }
        
        let pipeReadHandle = inputPipe.fileHandleForReading

        dup2(STDOUT_FILENO, outputPipe.fileHandleForWriting.fileDescriptor)
        
        dup2(inputPipe.fileHandleForWriting.fileDescriptor, STDOUT_FILENO)
//        dup2(inputPipe.fileHandleForWriting.fileDescriptor, STDERR_FILENO)

        NotificationCenter.default.addObserver(self, selector: #selector(self.handlePipeNotification), name: FileHandle.readCompletionNotification, object: pipeReadHandle)

        pipeReadHandle.readInBackgroundAndNotify(forModes: [RunLoop.Mode.common])
    }
    
    @objc func handlePipeNotification(notification: Notification) {
        inputPipe?.fileHandleForReading.readInBackgroundAndNotify(forModes: [RunLoop.Mode.common])
        
        if let data = notification.userInfo?[NSFileHandleNotificationDataItem] as? Data,
           let str = String(data: data, encoding: String.Encoding.ascii) {
            outputPipe?.fileHandleForWriting.write(data)
            outputText.append(str)
        }
    }
    
    private func stopListeningToStdout() {
        NotificationCenter.default.removeObserver(self)
    }
}
