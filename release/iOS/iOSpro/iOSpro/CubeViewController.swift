//
//  CubeViewController.swift
//  iOSpro — Bundle → Documents → SDK load；立方体 / Wolf / SU7 / Benz / 人模
//

import UIKit

enum ModelBrowseAsset {
    case cube
    case wolf
    case su7
    case benz
    case whiteMan
    case blackMan

    var title: String {
        switch self {
        case .cube: return "立方体"
        case .wolf: return "Wolf"
        case .su7: return "SU7"
        case .benz: return "Benz"
        case .whiteMan: return "白人"
        case .blackMan: return "黑人"
        }
    }
}

final class CubeViewController: UIViewController {
    private let asset: ModelBrowseAsset
    private let glView = ModelGLView(frame: .zero)
    private let hud = UILabel()
    private var pickBanner: PartPickBanner?
    private var pickHideWorkItem: DispatchWorkItem?
    private let forwardBtn = UIButton(type: .system)
    private let backBtn = UIButton(type: .system)
    private let leftBtn = UIButton(type: .system)
    private let rightBtn = UIButton(type: .system)
    private let sunBtn = UIButton(type: .system)
    private let torchBtn = UIButton(type: .system)
    private let lampBtn = UIButton(type: .system)
    private let controlBtn = UIButton(type: .system)
    private let explodeBtn = UIButton(type: .system)
    private var colorChips: [UIButton] = []
    private var colorRow: UIStackView!
    private var topBar: UIStackView!
    private var animDropdown: SCDropdownButton?
    private var holdTimer: Timer?
    private var holdTick: (() -> Void)?
    private var dragControlsLight = false
    private var exploded = false
    private var sunOn = true
    private var torchOn = true
    private var lampOn = true
    private var selectedColorIndex = 3  // 默认白
    private var controlsStack: UIStackView!

    /// 色卡：红 / 黄 / 天蓝 / 白
    private let lampColors: [(UIColor, Float, Float, Float)] = [
        (UIColor(red: 1, green: 0.18, blue: 0.12, alpha: 1), 1, 0.18, 0.12),
        (UIColor(red: 1, green: 0.85, blue: 0.2, alpha: 1), 1, 0.85, 0.2),
        (UIColor(red: 0.35, green: 0.72, blue: 1, alpha: 1), 0.35, 0.72, 1),
        (UIColor(white: 1, alpha: 1), 1, 1, 1),
    ]

    init(asset: ModelBrowseAsset) {
        self.asset = asset
        super.init(nibName: nil, bundle: nil)
    }

    @available(*, unavailable)
    required init?(coder: NSCoder) {
        fatalError("init(coder:) has not been implemented")
    }

    override func viewDidLoad() {
        super.viewDidLoad()
        title = asset.title
        view.backgroundColor = .black

        glView.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(glView)

        configureHoldButton(forwardBtn, title: "前进") { [weak self] in
            guard let self else { return }
            if self.dragControlsLight {
                self.glView.lightDolly(1)
            } else {
                self.glView.dolly(1)
            }
        }
        configureHoldButton(backBtn, title: "后退") { [weak self] in
            guard let self else { return }
            if self.dragControlsLight {
                self.glView.lightDolly(-1)
            } else {
                self.glView.dolly(-1)
            }
        }
        configureHoldButton(leftBtn, title: "左") { [weak self] in
            self?.glView.cameraPan(-1)
        }
        configureHoldButton(rightBtn, title: "右") { [weak self] in
            self?.glView.cameraPan(1)
        }

        configureStateButton(sunBtn, title: "太阳") { [weak self] in
            guard let self else { return }
            self.sunOn.toggle()
            self.glView.setDirLightEnabled(self.sunOn)
            self.refreshButtonStates()
        }
        configureStateButton(torchBtn, title: "手电") { [weak self] in
            guard let self else { return }
            self.torchOn.toggle()
            self.glView.setSpotLightEnabled(self.torchOn)
            self.refreshButtonStates()
        }
        // 灯：开/关点光源（与色卡同一行）
        configureStateButton(lampBtn, title: "灯") { [weak self] in
            guard let self else { return }
            self.lampOn.toggle()
            self.glView.setPointLightEnabled(self.lampOn)
            self.refreshButtonStates()
        }
        // 控制：与左右同行；切换拖灯 / 拖物体
        configureStateButton(controlBtn, title: "控制") { [weak self] in
            guard let self else { return }
            self.dragControlsLight.toggle()
            self.glView.dragControlsLight = self.dragControlsLight
            self.refreshButtonStates()
        }
        // 爆炸 / 复位：零件沿相对中心拉开；点选零件高亮
        configureStateButton(explodeBtn, title: "爆炸") { [weak self] in
            guard let self else { return }
            self.exploded.toggle()
            if self.exploded {
                self.glView.explode()
            } else {
                self.glView.implode()
            }
            self.refreshButtonStates()
        }

        let moveRow = UIStackView(arrangedSubviews: [
            forwardBtn, backBtn, leftBtn, rightBtn, controlBtn, explodeBtn,
        ])
        moveRow.axis = .horizontal
        moveRow.spacing = 8
        moveRow.alignment = .center

        colorRow = makeColorChipRow()
        let lampRow = UIStackView(arrangedSubviews: [sunBtn, torchBtn, lampBtn, colorRow])
        lampRow.axis = .horizontal
        lampRow.spacing = 8
        lampRow.alignment = .center
        controlsStack = lampRow

        let bar = UIStackView(arrangedSubviews: [moveRow, lampRow])
        bar.axis = .vertical
        bar.spacing = 8
        bar.alignment = .center
        bar.translatesAutoresizingMaskIntoConstraints = false
        view.addSubview(bar)
        topBar = bar

        hud.translatesAutoresizingMaskIntoConstraints = false
        hud.textColor = .white
        hud.numberOfLines = 0
        hud.font = .preferredFont(forTextStyle: .footnote)
        hud.textAlignment = .center
        view.addSubview(hud)

        // 每次点选初始化一个 PartPickBanner（工业蓝），中下展示 2s 后消失
        glView.onPick = { [weak self] meshId in
            self?.presentPickBanner(meshId: meshId)
        }

        NSLayoutConstraint.activate([
            glView.topAnchor.constraint(equalTo: view.topAnchor),
            glView.leadingAnchor.constraint(equalTo: view.leadingAnchor),
            glView.trailingAnchor.constraint(equalTo: view.trailingAnchor),
            glView.bottomAnchor.constraint(equalTo: view.bottomAnchor),

            bar.topAnchor.constraint(equalTo: view.safeAreaLayoutGuide.topAnchor, constant: 12),
            bar.centerXAnchor.constraint(equalTo: view.centerXAnchor),

            hud.leadingAnchor.constraint(equalTo: view.layoutMarginsGuide.leadingAnchor),
            hud.trailingAnchor.constraint(equalTo: view.layoutMarginsGuide.trailingAnchor),
            hud.bottomAnchor.constraint(equalTo: view.safeAreaLayoutGuide.bottomAnchor, constant: -12),
        ])

        switch prepareAndLoad() {
        case .success(let info):
            if glView.animationCount() > 0 {
                setupAnimDropdown(below: bar)
            }
            hud.text = info
        case .failure(let message):
            hud.text = message
        }
        applySelectedLampColor()
        refreshButtonStates()
    }

    /// 点击命中：初始化反馈对象；未中则清高亮并不弹条。
    private func presentPickBanner(meshId: Int) {
        pickHideWorkItem?.cancel()
        pickBanner?.dismiss(animated: false)
        pickBanner = nil

        guard meshId >= 0 else { return }

        // 每次点击新建一个对象（配件选中反馈）
        let banner = PartPickBanner(meshId: meshId)
        banner.attach(to: view)
        pickBanner = banner

        let work = DispatchWorkItem { [weak self, weak banner] in
            guard let self, let banner, self.pickBanner === banner else { return }
            banner.dismiss(animated: true) { [weak self] in
                if self?.pickBanner === banner {
                    self?.pickBanner = nil
                }
            }
        }
        pickHideWorkItem = work
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0, execute: work)
    }

    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        applyDarkNavBar()
        glView.resumeRendering()
    }

    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        // 必须停 DisplayLink：否则 CADisplayLink 强持有 GLView，退页后仍 60fps → 发烫/泄漏
        glView.pauseRendering()
        stopHold()
        pickHideWorkItem?.cancel()
        pickBanner?.dismiss(animated: false)
        pickBanner = nil
        animDropdown?.dismiss()
        navigationController?.navigationBar.tintColor = nil
        navigationItem.standardAppearance = nil
        navigationItem.scrollEdgeAppearance = nil
        navigationItem.compactAppearance = nil
    }

    deinit {
        glView.shutdown()
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

    private enum LoadResult {
        case success(String)
        case failure(String)
    }

    private func prepareAndLoad() -> LoadResult {
        do {
            let path: String
            switch asset {
            case .cube: path = try copyCubeToDocuments()
            case .wolf: path = try copyWolfToDocuments()
            case .su7: path = try copySu7ToDocuments()
            case .benz: path = try copyBenzToDocuments()
            case .whiteMan: path = try copyGlbToDocuments(stem: "whiteMan", folder: "whiteMan")
            case .blackMan: path = try copyGlbToDocuments(stem: "blackMan", folder: "blackMan")
            }
            guard glView.loadModel(atPath: path) else {
                return .failure("Assimp load failed: \(path)")
            }
            let n = glView.animationCount()
            return .success("Documents \(asset.title) — \(n) clips · tap 选件 · 爆炸拆解")
        } catch {
            return .failure("Bundle→Documents failed: \(error.localizedDescription)")
        }
    }

    private func documentsModelsDir() throws -> URL {
        let fm = FileManager.default
        guard let docs = fm.urls(for: .documentDirectory, in: .userDomainMask).first else {
            throw NSError(domain: "ModelBrowse", code: 1,
                          userInfo: [NSLocalizedDescriptionKey: "no Documents"])
        }
        let dir = docs.appendingPathComponent("models", isDirectory: true)
        try fm.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir
    }

    private func copyCubeToDocuments() throws -> String {
        let fm = FileManager.default
        let dir = try documentsModelsDir().appendingPathComponent("cube", isDirectory: true)
        try fm.createDirectory(at: dir, withIntermediateDirectories: true)
        let names = ["cube.obj", "cube.mtl", "box.png"]
        for name in names {
            let base = (name as NSString).deletingPathExtension
            let ext = (name as NSString).pathExtension
            guard let src = bundleURL(resource: base, ext: ext, subdirs: ["models", ""]) else {
                throw NSError(domain: "ModelBrowse", code: 2,
                              userInfo: [NSLocalizedDescriptionKey: "missing \(name)"])
            }
            let dest = dir.appendingPathComponent(name)
            if fm.fileExists(atPath: dest.path) { try fm.removeItem(at: dest) }
            if name.hasSuffix(".png"),
               let image = UIImage(contentsOfFile: src.path),
               let png = image.pngData() {
                try png.write(to: dest, options: .atomic)
            } else {
                try fm.copyItem(at: src, to: dest)
            }
        }
        return dir.appendingPathComponent("cube.obj").path
    }

    private func copyWolfToDocuments() throws -> String {
        let fm = FileManager.default
        let destRoot = try documentsModelsDir().appendingPathComponent("Wolf-fbx", isDirectory: true)
        let destTex = destRoot.appendingPathComponent("textures", isDirectory: true)
        let fbxStem = "Wolf_One_fbx7.4_binary"
        let fbxName = "\(fbxStem).fbx"

        guard let fbxSrc = bundleURL(resource: fbxStem, ext: "fbx", subdirs: ["models/Wolf-fbx", "models", ""]) else {
            throw NSError(domain: "ModelBrowse", code: 3,
                          userInfo: [NSLocalizedDescriptionKey: "fbx not in Bundle"])
        }

        try fm.createDirectory(at: destTex, withIntermediateDirectories: true)
        let destFbx = destRoot.appendingPathComponent(fbxName)
        if fm.fileExists(atPath: destFbx.path) { try fm.removeItem(at: destFbx) }
        try fm.copyItem(at: fbxSrc, to: destFbx)

        let textures = ["Wolf_Body", "Wolf_Eyes_1", "Wolf_Eyes_2", "Wolf_Fur"]
        for name in textures {
            guard let src = bundleURL(resource: name, ext: "jpg",
                                      subdirs: ["models/Wolf-fbx/textures", "models/Wolf-fbx", "models", ""]) else {
                throw NSError(domain: "ModelBrowse", code: 4,
                              userInfo: [NSLocalizedDescriptionKey: "missing texture \(name).jpg"])
            }
            let dest = destTex.appendingPathComponent("\(name).jpg")
            if fm.fileExists(atPath: dest.path) { try fm.removeItem(at: dest) }
            try fm.copyItem(at: src, to: dest)
        }
        return destFbx.path
    }

    private func copySu7ToDocuments() throws -> String {
        let fm = FileManager.default
        let dir = try documentsModelsDir().appendingPathComponent("su7", isDirectory: true)
        try fm.createDirectory(at: dir, withIntermediateDirectories: true)
        guard let src = bundleURL(resource: "su7", ext: "glb", subdirs: ["models", ""]) else {
            throw NSError(domain: "ModelBrowse", code: 5,
                          userInfo: [NSLocalizedDescriptionKey: "su7.glb not in Bundle"])
        }
        let dest = dir.appendingPathComponent("su7.glb")
        if fm.fileExists(atPath: dest.path) {
            let srcSize = (try? src.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? -1
            let dstSize = (try? dest.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? -2
            if srcSize == dstSize { return dest.path }
            try fm.removeItem(at: dest)
        }
        try fm.copyItem(at: src, to: dest)
        return dest.path
    }

    private func copyBenzToDocuments() throws -> String {
        try copyGlbToDocuments(stem: "benz_amg", folder: "benz")
    }

    /// Bundle models/<stem>.glb → Documents/models/<folder>/<stem>.glb
    private func copyGlbToDocuments(stem: String, folder: String) throws -> String {
        let fm = FileManager.default
        let dir = try documentsModelsDir().appendingPathComponent(folder, isDirectory: true)
        try fm.createDirectory(at: dir, withIntermediateDirectories: true)
        guard let src = bundleURL(resource: stem, ext: "glb", subdirs: ["models", ""]) else {
            throw NSError(domain: "ModelBrowse", code: 6,
                          userInfo: [NSLocalizedDescriptionKey: "\(stem).glb not in Bundle"])
        }
        let dest = dir.appendingPathComponent("\(stem).glb")
        // 大 glb（Benz ~40MB）已存在则复用，避免反复拷贝撑内存/闪存
        if fm.fileExists(atPath: dest.path) {
            let srcSize = (try? src.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? -1
            let dstSize = (try? dest.resourceValues(forKeys: [.fileSizeKey]).fileSize) ?? -2
            if srcSize == dstSize { return dest.path }
            try fm.removeItem(at: dest)
        }
        try fm.copyItem(at: src, to: dest)
        return dest.path
    }

    private func bundleURL(resource: String, ext: String, subdirs: [String]) -> URL? {
        for sub in subdirs {
            let url: URL?
            if sub.isEmpty {
                url = Bundle.main.url(forResource: resource, withExtension: ext)
            } else {
                url = Bundle.main.url(forResource: resource, withExtension: ext, subdirectory: sub)
            }
            if let url, FileManager.default.fileExists(atPath: url.path) { return url }
        }
        return nil
    }

    private func setupAnimDropdown(below stack: UIStackView) {
        var names = glView.animationNames()
        if names.isEmpty { names = ["—"] }
        let dropdown = SCDropdownButton(prefix: "Anim", options: names, selectedIndex: 0)
        dropdown.panelAlignment = .leading
        dropdown.isEnabled = glView.animationCount() > 0
        dropdown.selectionHandler = { [weak self] index, _ in
            guard let self else { return }
            _ = self.glView.playAnimation(at: index)
        }
        view.addSubview(dropdown)
        NSLayoutConstraint.activate([
            dropdown.leadingAnchor.constraint(equalTo: view.layoutMarginsGuide.leadingAnchor),
            dropdown.topAnchor.constraint(equalTo: stack.bottomAnchor, constant: 10),
            dropdown.heightAnchor.constraint(equalToConstant: 36),
        ])
        animDropdown = dropdown
    }

    private func configureStateButton(_ button: UIButton, title: String, action: @escaping () -> Void) {
        styleControlButton(button)
        button.setTitle(title, for: .normal)
        button.addAction(UIAction { _ in action() }, for: .touchUpInside)
    }

    private func makeColorChipRow() -> UIStackView {
        var chips: [UIView] = []
        for (i, entry) in lampColors.enumerated() {
            let chip = UIButton(type: .custom)
            chip.backgroundColor = entry.0
            chip.layer.cornerRadius = 4
            chip.layer.borderWidth = 2
            chip.layer.borderColor = UIColor.white.withAlphaComponent(0.35).cgColor
            chip.tag = i
            chip.translatesAutoresizingMaskIntoConstraints = false
            NSLayoutConstraint.activate([
                chip.widthAnchor.constraint(equalToConstant: 28),
                chip.heightAnchor.constraint(equalToConstant: 28),
            ])
            chip.addAction(UIAction { [weak self] _ in
                guard let self else { return }
                self.selectedColorIndex = i
                self.applySelectedLampColor()
                self.refreshButtonStates()
            }, for: .touchUpInside)
            colorChips.append(chip)
            chips.append(chip)
        }
        let row = UIStackView(arrangedSubviews: chips)
        row.axis = .horizontal
        row.spacing = 10
        row.alignment = .center
        return row
    }

    private func applySelectedLampColor() {
        let c = lampColors[selectedColorIndex]
        glView.setPointLightColorR(c.1, g: c.2, b: c.3)
    }

    /// 开/选中：亮底；关：暗底。色卡与「灯」同行，灯关时色卡半透明仍占位。
    private func refreshButtonStates() {
        applyToggleStyle(sunBtn, on: sunOn)
        applyToggleStyle(torchBtn, on: torchOn)
        applyToggleStyle(lampBtn, on: lampOn)
        applyToggleStyle(controlBtn, on: dragControlsLight)
        explodeBtn.setTitle(exploded ? "复位" : "爆炸", for: .normal)
        applyToggleStyle(explodeBtn, on: exploded)
        colorRow?.alpha = lampOn ? 1 : 0.35
        colorRow?.isUserInteractionEnabled = lampOn
        for (i, chip) in colorChips.enumerated() {
            let selected = (i == selectedColorIndex)
            chip.layer.borderWidth = selected ? 3 : 1
            chip.layer.borderColor = selected
                ? UIColor.white.cgColor
                : UIColor.white.withAlphaComponent(0.3).cgColor
            chip.transform = selected ? CGAffineTransform(scaleX: 1.12, y: 1.12) : .identity
        }
    }

    private func applyToggleStyle(_ button: UIButton, on: Bool) {
        button.backgroundColor = on
            ? UIColor(white: 1, alpha: 0.28)
            : UIColor(white: 0, alpha: 0.45)
        button.setTitleColor(on ? .white : UIColor(white: 1, alpha: 0.55), for: .normal)
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
        button.contentEdgeInsets = UIEdgeInsets(top: 8, left: 14, bottom: 8, right: 14)
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

/// 点选反馈：每次点击初始化一个实例，中下展示「配件ID：」，工业蓝，约 2s 消失。
final class PartPickBanner {
    let meshId: Int
    private let label = UILabel()

    /// CAD 常见选中蓝
    private static let industrialBlue = UIColor(red: 0.20, green: 0.48, blue: 0.98, alpha: 1)

    init(meshId: Int) {
        self.meshId = meshId
        label.text = "  配件ID：\(meshId)  "
        label.textColor = .white
        label.font = .systemFont(ofSize: 16, weight: .semibold)
        label.textAlignment = .center
        label.backgroundColor = Self.industrialBlue.withAlphaComponent(0.92)
        label.layer.cornerRadius = 10
        label.clipsToBounds = true
        label.alpha = 0
        label.translatesAutoresizingMaskIntoConstraints = false
    }

    func attach(to host: UIView) {
        host.addSubview(label)
        // 屏幕中下：相对安全区中心下移
        let offset = max(host.bounds.height * 0.18, 120)
        NSLayoutConstraint.activate([
            label.centerXAnchor.constraint(equalTo: host.centerXAnchor),
            label.centerYAnchor.constraint(equalTo: host.safeAreaLayoutGuide.centerYAnchor, constant: offset),
            label.heightAnchor.constraint(greaterThanOrEqualToConstant: 40),
            label.widthAnchor.constraint(greaterThanOrEqualToConstant: 140),
        ])
        host.layoutIfNeeded()
        UIView.animate(withDuration: 0.18) {
            self.label.alpha = 1
        }
    }

    func dismiss(animated: Bool, completion: (() -> Void)? = nil) {
        let finish = {
            self.label.removeFromSuperview()
            completion?()
        }
        guard animated else {
            finish()
            return
        }
        UIView.animate(withDuration: 0.25, animations: {
            self.label.alpha = 0
        }, completion: { _ in
            finish()
        })
    }
}
