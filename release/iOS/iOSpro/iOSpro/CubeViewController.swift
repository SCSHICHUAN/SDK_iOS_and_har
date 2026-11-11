//
//  CubeViewController.swift
//  iOSpro — GLES cube browse (前进 / 后退 / 控制)
//

import UIKit

final class CubeViewController: UIViewController {
    private let glView = ModelGLView(frame: .zero)
    private let hud = UILabel()
    private let forwardBtn = UIButton(type: .system)
    private let backBtn = UIButton(type: .system)
    private let controlBtn = UIButton(type: .system)
    private var holdTimer: Timer?
    private var holdTick: (() -> Void)?
    private var dragControlsLight = false

    override func viewDidLoad() {
        super.viewDidLoad()
        title = "立方体"
        view.backgroundColor = .black

        glView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(glView)

        configureHoldButton(forwardBtn, title: "前进") { [weak self] in self?.glView.dolly(1) }
        configureHoldButton(backBtn, title: "后退") { [weak self] in self?.glView.dolly(-1) }
        configureToggleButton(controlBtn)

        let stack = UIStackView(arrangedSubviews: [forwardBtn, backBtn, controlBtn])
        stack.axis = .horizontal
        stack.spacing = 12
        stack.alignment = .center
        stack.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(stack)

        hud.translatesAutoresizingMaskIntoConstraints = false
        hud.textColor = .white
        hud.numberOfLines = 0
        hud.font = .preferredFont(forTextStyle: .footnote)
        hud.textAlignment = .center
        view.addSubview(hud)

        NSLayoutConstraint.activate([
            glView.topAnchor.constraint(equalTo: view.topAnchor),
            glView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            glView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            glView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

            stack.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 12),
            stack.centerXAnchor.constraint(equalTo: view.centerXAnchor),

            hud.leadingAnchor.constraint(equalTo: view.layoutMarginsGuide.leadingAnchor),
            hud.trailingAnchor.constraint(equalTo: view.layoutMarginsGuide.trailingAnchor),
            hud.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -12),
        ])

        if let modelPath = copyModelsToCaches(), glView.loadModel(atPath: modelPath) {
            hud.text = "loaded cube.obj + box.png — drag orbit, pinch zoom"
        } else {
            hud.text = "load failed"
        }
        refreshControlTitle()
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        applyDarkNavBar()
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        // Restore default (list) nav chrome when popping back.
        navigationController?.navigationBar.tintColor = nil
        navigationItem.standardAppearance = nil
        navigationItem.scrollEdgeAppearance = nil
        navigationItem.compactAppearance = nil
    }

    private func applyDarkNavBar() {
        let appearance = UINavigationBarAppearance()
        appearance.configureWithTransparentBackground()
        appearance.backgroundColor = UIColor(white: 0, alpha: 0.45)
        appearance.titleTextAttributes = [.foregroundColor: UIColor.white]
        appearance.largeTitleTextAttributes = [.foregroundColor: UIColor.white]
        navigationItem.standardAppearance = appearance
        navigationItem.scrollEdgeAppearance = appearance
        navigationItem.compactAppearance = appearance
        navigationController?.navigationBar.tintColor = .white
    }

    private func copyModelsToCaches() -> String? {
        let names = ["cube.obj", "cube.mtl", "box.png"]
        guard let caches = FileManager.default.urls(for: .cachesDirectory, in: .userDomainMask).first else {
            return nil
        }
        let dir = caches.appendingPathComponent("models", isDirectory: true)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        for name in names {
            let dest = dir.appendingPathComponent(name)
            let base = (name as NSString).deletingPathExtension
            let ext = (name as NSString).pathExtension
            let src = Bundle.main.url(forResource: base, withExtension: ext, subdirectory: "models")
                ?? Bundle.main.url(forResource: base, withExtension: ext)
            guard let src else { return nil }
            try? FileManager.default.removeItem(at: dest)
            do {
                if name.hasSuffix(".png") {
                    guard let image = UIImage(contentsOfFile: src.path),
                          let png = image.pngData() else { return nil }
                    try png.write(to: dest, options: .atomic)
                } else {
                    try FileManager.default.copyItem(at: src, to: dest)
                }
            } catch {
                return nil
            }
        }
        return dir.appendingPathComponent("cube.obj").path
    }

    private func configureToggleButton(_ button: UIButton) {
        styleControlButton(button)
        button.addAction(UIAction { [weak self] _ in
            guard let self else { return }
            self.dragControlsLight.toggle()
            self.glView.dragControlsLight = self.dragControlsLight
            self.refreshControlTitle()
        }, for: .touchUpInside)
    }

    private func refreshControlTitle() {
        controlBtn.setTitle(dragControlsLight ? "控制:灯" : "控制:物体", for: .normal)
    }

    private func configureHoldButton(_ button: UIButton, title: String, tick: @escaping () -> Void) {
        styleControlButton(button)
        button.setTitle(title, for: .normal)
        button.addAction(UIAction { [weak self] _ in
            self?.startHold(tick)
        }, for: .touchDown)
        let stop = UIAction { [weak self] _ in
            self?.stopHold()
        }
        button.addAction(stop, for: .touchUpInside)
        button.addAction(stop, for: .touchUpOutside)
        button.addAction(stop, for: .touchCancel)
    }

    private func styleControlButton(_ button: UIButton) {
        button.setTitleColor(.white, for: .normal)
        button.backgroundColor = UIColor(white: 0, alpha: 0.45)
        button.contentEdgeInsets = UIEdgeInsets(top: 8, left: 16, bottom: 8, right: 16)
        button.layer.cornerRadius = 8
    }

    private func startHold(_ tick: @escaping () -> Void) {
        holdTick = tick
        tick()
        holdTimer?.invalidate()
        let timer = Timer(timeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            self?.holdTick?()
        }
        RunLoop.main.add(timer, forMode: .common)
        holdTimer = timer
    }

    private func stopHold() {
        holdTimer?.invalidate()
        holdTimer = nil
        holdTick = nil
    }
}
