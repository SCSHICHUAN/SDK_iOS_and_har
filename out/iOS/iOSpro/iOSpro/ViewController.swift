//
//  ViewController.swift
//  iOSpro
//

import UIKit
import Core

class ViewController: UIViewController {

    private let label = UILabel()

    override func viewDidLoad() {
        super.viewDidLoad()
        view.backgroundColor = .systemBackground

        label.translatesAutoresizingMaskIntoConstraints = false
        label.textAlignment = .center
        label.numberOfLines = 0
        label.font = .preferredFont(forTextStyle: .title2)
        view.addSubview(label)

        NSLayoutConstraint.activate([
            label.leadingAnchor.constraint(equalTo: view.layoutMarginsGuide.leadingAnchor),
            label.trailingAnchor.constraint(equalTo: view.layoutMarginsGuide.trailingAnchor),
            label.centerYAnchor.constraint(equalTo: view.centerYAnchor),
        ])

        // Call shared C API from Core.xcframework
        let code = core_hello_world()
        let text = String(cString: core_hello_world_string())
        label.text = code == 0
            ? "core_hello_world() => \(code)\ncore_hello_world_string() => \(text)"
            : "core_hello_world() failed: \(code)"
    }
}
