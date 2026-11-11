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
import androidx.compose.material3.ExperimentalMaterial3Api
import androidx.compose.material3.Icon
import androidx.compose.material3.IconButton
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.material3.CenterAlignedTopAppBar
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
import androidx.compose.ui.text.style.TextAlign
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import androidx.compose.ui.viewinterop.AndroidView
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import java.io.File
import kotlinx.coroutines.delay

private val IosGroupedBg = Color(0xFFF2F2F7)
private val IosCellBg = Color.White
private val IosLabel = Color.Black
private val IosChevron = Color(0xFFC7C7CC)
private val IosNavBg = Color(0xFFF9F9F9)
private val IosChipBg = Color.Black.copy(alpha = 0.45f)

/**
 * Mainstream Android:
 *   NavHost (≈ UINavigationController)
 *     ├─ list  (LazyColumn ≈ UITableView)
 *     └─ cube  (push with slide)
 */
class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        val modelPath = copyAssetModel()

        setContent {
            val nav = rememberNavController()
            var didAutoPush by remember { mutableStateOf(false) }
            LaunchedEffect(Unit) {
                if (!didAutoPush) {
                    didAutoPush = true
                    nav.navigate("cube")
                }
            }
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
                    RootListScreen(onOpenCube = { nav.navigate("cube") })
                }
                composable("cube") {
                    CubeScreen(
                        modelPath = modelPath,
                        onBack = { nav.popBackStack() },
                    )
                }
            }
        }
    }

    private fun copyAssetModel(): String {
        for (name in listOf("cube.obj", "cube.mtl", "box.png")) {
            assets.open("models/$name").use { input ->
                File(filesDir, name).outputStream().use { output -> input.copyTo(output) }
            }
        }
        return File(filesDir, "cube.obj").absolutePath
    }
}

@OptIn(ExperimentalMaterial3Api::class)
@Composable
private fun RootListScreen(onOpenCube: () -> Unit) {
    val rows = listOf("立方体")
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
                    rows.forEach { title ->
                        Row(
                            modifier = Modifier
                                .fillMaxWidth()
                                .height(48.dp)
                                .clickable { onOpenCube() }
                                .padding(horizontal = 16.dp),
                            verticalAlignment = Alignment.CenterVertically,
                        ) {
                            Text(
                                text = title,
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
private fun CubeScreen(modelPath: String, onBack: () -> Unit) {
    var glView by remember { mutableStateOf<ModelGLSurfaceView?>(null) }
    var dragLight by remember { mutableStateOf(false) }
    val hud = "loaded cube.obj + box.png — drag orbit, pinch zoom"

    // Like iOS: GL is full-bleed; translucent system bar + chips/HUD overlay — no black bands.
    Box(Modifier.fillMaxSize().background(Color.Black)) {
        AndroidView(
            factory = { ctx ->
                ModelGLSurfaceView(ctx).also {
                    it.setModelPath(modelPath)
                    glView = it
                }
            },
            update = { view -> view.dragControlsLight = dragLight },
            modifier = Modifier.fillMaxSize(),
        )
        Scaffold(
            contentWindowInsets = WindowInsets(0, 0, 0, 0),
            topBar = {
                CenterAlignedTopAppBar(
                    title = { Text("立方体", color = Color.White) },
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
                    horizontalArrangement = Arrangement.spacedBy(12.dp),
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
private fun IosChipButton(label: String, hold: Boolean, onTick: () -> Unit) {
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
        color = Color.White,
        fontSize = 14.sp,
        modifier = Modifier
            .clip(RoundedCornerShape(8.dp))
            .background(IosChipBg)
            .clickable(
                interactionSource = interaction,
                indication = null,
                onClick = { onTick() },
            )
            .padding(horizontal = 16.dp, vertical = 8.dp),
    )
}
