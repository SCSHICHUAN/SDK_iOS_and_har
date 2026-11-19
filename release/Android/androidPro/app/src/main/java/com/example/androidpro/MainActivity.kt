package com.example.androidpro

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.animation.AnimatedContentTransitionScope
import androidx.compose.animation.core.tween
import androidx.compose.animation.fadeIn
import androidx.compose.animation.fadeOut
import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.interaction.MutableInteractionSource
import androidx.compose.foundation.interaction.collectIsPressedAsState
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.navigationBarsPadding
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.automirrored.filled.ArrowBack
import androidx.compose.material.icons.automirrored.filled.KeyboardArrowRight
import androidx.compose.material3.CenterAlignedTopAppBar
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.TopAppBarDefaults
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.key
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.navigation.NavType
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import androidx.navigation.navArgument
import java.io.File
import kotlinx.coroutines.delay

private val IosGroupedBg = Color(0xFFF2F2F7)
private val IosCellBg = Color.White
private val IosLabel = Color.Black
private val IosChevron = Color(0xFFC7C7CC)
private val IosNavBg = Color(0xFFF9F9F9)
private val IosChipBg = Color.Black.copy(alpha = 0.45f)
/** CAD / industrial selection blue */
private val IndustrialBlue = Color(0xFF337AFB)

private enum class BrowseAsset(val route: String, val title: String) {
    Cube("cube", "立方体"),
    Su7("su7", "SU7"),
    Benz("benz", "Benz"),
}

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            val nav = rememberNavController()
            NavHost(
                navController = nav,
                startDestination = "list",
                enterTransition = {
                    slideIntoContainer(
                        towards = AnimatedContentTransitionScope.SlideDirection.Left,
                        animationSpec = tween(320),
                    ) + fadeIn(tween(320))
                },
                exitTransition = {
                    slideOutOfContainer(
                        towards = AnimatedContentTransitionScope.SlideDirection.Left,
                        animationSpec = tween(320),
                        targetOffset = { it / 4 },
                    ) + fadeOut(tween(320))
                },
                popEnterTransition = {
                    slideIntoContainer(
                        towards = AnimatedContentTransitionScope.SlideDirection.Right,
                        animationSpec = tween(320),
                        initialOffset = { it / 4 },
                    ) + fadeIn(tween(320))
                },
                popExitTransition = {
                    slideOutOfContainer(
                        towards = AnimatedContentTransitionScope.SlideDirection.Right,
                        animationSpec = tween(320),
                    ) + fadeOut(tween(320))
                },
            ) {
                composable("list") {
                    RootListScreen(onOpen = { asset -> nav.navigate("browse/${asset.route}") })
                }
                composable(
                    route = "browse/{asset}",
                    arguments = listOf(navArgument("asset") { type = NavType.StringType }),
                ) { entry ->
                    val key = entry.arguments?.getString("asset") ?: "cube"
                    val asset = BrowseAsset.entries.find { it.route == key } ?: BrowseAsset.Cube
                    BrowseScreen(
                        asset = asset,
                        onBack = { nav.popBackStack() },
                    )
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun RootListScreen(onOpen: (BrowseAsset) -> Unit) {
    val rows = BrowseAsset.entries
    Scaffold(
        topBar = {
            CenterAlignedTopAppBar(
                title = { Text("androidPro") },
                colors = TopAppBarDefaults.topAppBarColors(
                    containerColor = IosNavBg,
                    titleContentColor = IosLabel,
                ),
            )
        },
        containerColor = IosGroupedBg,
    ) { inner ->
        LazyColumn(
            modifier = Modifier
                .fillMaxSize()
                .padding(inner)
                .padding(horizontal = 16.dp, vertical = 12.dp),
        ) {
            item {
                Column(
                    modifier = Modifier
                        .fillMaxWidth()
                        .clip(RoundedCornerShape(10.dp))
                        .background(IosCellBg),
                ) {
                    rows.forEach { asset ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(48.dp)
                                .clickable { onOpen(asset) }
                                .padding(horizontal = 16.dp),
                            verticalAlignment = Alignment.CenterVertically,
                        ) {
                            Text(
                                text = asset.title,
                                color = IosLabel,
                                fontSize = 17.sp,
                                modifier = Modifier.weight(1f),
                            )
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.KeyboardArrowRight,
                                contentDescription = null,
                                tint = IosChevron,
                            )
                        }
                    }
                }
            }
        }
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun BrowseScreen(asset: BrowseAsset, onBack: () -> Unit) {
    val context = LocalContext.current
    val modelPath = remember(asset) { copyAssetModel(context, asset) }
    var glView by remember { mutableStateOf<ModelGLSurfaceView?>(null) }
    var dragLight by remember { mutableStateOf(false) }
    var exploded by remember { mutableStateOf(false) }
    var pickBannerId by remember { mutableStateOf<Int?>(null) }
    val hud = remember(asset) {
        when (asset) {
            BrowseAsset.Cube -> "loaded cube — tap 选件 · 爆炸拆解"
            BrowseAsset.Su7 -> "Documents SU7 — tap 选件 · 爆炸拆解"
            BrowseAsset.Benz -> "Documents Benz — tap 选件 · 爆炸拆解"
        }
    }

    LaunchedEffect(pickBannerId) {
        val id = pickBannerId ?: return@LaunchedEffect
        delay(2000)
        if (pickBannerId == id) pickBannerId = null
    }

    Box(Modifier.fillMaxSize().background(Color.Black)) {
        AndroidView(
            factory = { ctx ->
                ModelGLSurfaceView(ctx).also { view ->
                    view.setModelPath(modelPath)
                    view.onPick = { meshId ->
                        if (meshId >= 0) {
                            pickBannerId = meshId
                        } else {
                            view.clearSelection()
                            pickBannerId = null
                        }
                    }
                    glView = view
                }
            },
            update = { view ->
                view.dragControlsLight = dragLight
                view.onPick = { meshId ->
                    if (meshId >= 0) {
                        pickBannerId = meshId
                    } else {
                        view.clearSelection()
                        pickBannerId = null
                    }
                }
            },
            modifier = Modifier.fillMaxSize(),
        )
        Scaffold(
            contentWindowInsets = WindowInsets(0, 0, 0, 0),
            topBar = {
                CenterAlignedTopAppBar(
                    title = { Text(asset.title, color = Color.White) },
                    navigationIcon = {
                        IconButton(onClick = onBack) {
                            Icon(
                                imageVector = Icons.AutoMirrored.Filled.ArrowBack,
                                contentDescription = "返回",
                                tint = Color.White,
                            )
                        }
                    },
                    colors = TopAppBarDefaults.topAppBarColors(
                        containerColor = IosChipBg,
                        titleContentColor = Color.White,
                        navigationIconContentColor = Color.White,
                    ),
                )
            },
            containerColor = Color.Transparent,
        ) { inner ->
            Box(
                Modifier
                    .fillMaxSize()
                    .padding(inner),
            ) {
                Row(
                    modifier = Modifier
                        .align(Alignment.TopCenter)
                        .padding(top = 8.dp),
                    horizontalArrangement = Arrangement.spacedBy(8.dp),
                ) {
                    IosChipButton(label = "前进", hold = true) { glView?.dolly(1f) }
                    IosChipButton(label = "后退", hold = true) { glView?.dolly(-1f) }
                    key(if (dragLight) "light" else "object") {
                        IosChipButton(
                            label = if (dragLight) "控制:灯" else "控制:物体",
                            hold = false,
                        ) {
                            dragLight = !dragLight
                            glView?.dragControlsLight = dragLight
                        }
                    }
                    key(if (exploded) "reset" else "explode") {
                        IosChipButton(
                            label = if (exploded) "复位" else "爆炸",
                            hold = false,
                            active = exploded,
                        ) {
                            exploded = !exploded
                            if (exploded) glView?.explode(1f) else glView?.implode()
                        }
                    }
                }

                // Mid-lower industrial-blue banner; new id each pick (≈ PartPickBanner)
                pickBannerId?.let { meshId ->
                    Text(
                        text = "  配件ID：$meshId  ",
                        color = Color.White,
                        fontSize = 16.sp,
                        fontWeight = FontWeight.SemiBold,
                        modifier = Modifier
                            .align(Alignment.Center)
                            .padding(top = 120.dp)
                            .clip(RoundedCornerShape(10.dp))
                            .background(IndustrialBlue.copy(alpha = 0.92f))
                            .padding(horizontal = 16.dp, vertical = 10.dp),
                    )
                }

                Text(
                    text = hud,
                    color = Color.White,
                    fontSize = 13.sp,
                    textAlign = TextAlign.Center,
                    modifier = Modifier
                        .align(Alignment.BottomCenter)
                        .navigationBarsPadding()
                        .padding(16.dp),
                )
            }
        }
    }
}

@Composable
private fun IosChipButton(
    label: String,
    hold: Boolean,
    active: Boolean = false,
    onTick: () -> Unit,
) {
    val interaction = remember { MutableInteractionSource() }
    val pressed by interaction.collectIsPressedAsState()
    LaunchedEffect(pressed, hold) {
        if (!hold) return@LaunchedEffect
        while (pressed) {
            onTick()
            delay(33)
        }
    }
    Text(
        text = label,
        color = if (active) Color.White else Color.White.copy(alpha = 0.9f),
        fontSize = 14.sp,
        modifier = Modifier
            .clip(RoundedCornerShape(8.dp))
            .background(if (active) Color.White.copy(alpha = 0.28f) else IosChipBg)
            .clickable(
                interactionSource = interaction,
                indication = null,
                onClick = { onTick() },
            )
            .padding(horizontal = 14.dp, vertical = 8.dp),
    )
}

private fun copyAssetModel(context: android.content.Context, asset: BrowseAsset): String {
    val files = context.filesDir
    return when (asset) {
        BrowseAsset.Cube -> {
            for (name in listOf("cube.obj", "cube.mtl", "box.png")) {
                context.assets.open("models/$name").use { input ->
                    File(files, name).outputStream().use { output -> input.copyTo(output) }
                }
            }
            File(files, "cube.obj").absolutePath
        }
        BrowseAsset.Su7 -> copyGlb(context, "su7.glb", "su7")
        BrowseAsset.Benz -> copyGlb(context, "benz_amg.glb", "benz")
    }
}

private fun copyGlb(context: android.content.Context, assetName: String, folder: String): String {
    val dir = File(context.filesDir, "models/$folder").apply { mkdirs() }
    val dest = File(dir, assetName)
    context.assets.open("models/$assetName").use { input ->
        val srcSize = input.available().toLong()
        if (dest.exists() && dest.length() == srcSize) return dest.absolutePath
        dest.outputStream().use { output -> input.copyTo(output) }
    }
    return dest.absolutePath
}
