package com.example.androidpro

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Spacer
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Button
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.*
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.tooling.preview.Preview
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.example.androidpro.ui.theme.AndroidProTheme
import kotlinx.coroutines.delay

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            AndroidProTheme {
                // ===== 行业主流启动页 =====
                var showSplash by remember { mutableStateOf(true) }
                var isLoggedIn by remember { mutableStateOf(false) } // 模拟登录态（以后从本地读）

                // 双条件跳转：初始化完成 && 至少展示 1.5 秒
                LaunchedEffect(Unit) {
                    val startTime = System.currentTimeMillis()
                    initApp()                                  // ① 后台初始化（以后填真实逻辑）
                    val elapsed = System.currentTimeMillis() - startTime
                    val rest = 1500L - elapsed                 // ② 保底展示时间
                    if (rest > 0) delay(rest)
                    showSplash = false
                }

                when {
                    showSplash -> SplashScreen()               // 启动页：苹果 LaunchScreen 风格
                    isLoggedIn -> Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                        FirstScreen(modifier = Modifier.padding(innerPadding))
                    }
                    else -> LoginScreen()                      // 未登录 → 登录页
                }
            }
        }
    }

    // 初始化：行业主流里这里放 SDK 初始化 / 登录检查 / 拉配置 / 预加载
    private suspend fun initApp() {
        delay(300) // 模拟初始化耗时，以后换成真实任务
    }
}

@Composable
fun SplashScreen() {
    // 启动页：白底 + 居中品牌名，和苹果 LaunchScreen 一样
    Box(
        modifier = Modifier.fillMaxSize().background(Color.White),
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = "androidPro",
            fontSize = 32.sp,
            fontWeight = FontWeight.Bold,
            color = Color(0xFF1F2328)
        )
    }
}

@Composable
fun FirstScreen(modifier: Modifier = Modifier) {
    var count by remember { mutableStateOf(0) }

    Column(
        modifier = modifier.fillMaxSize().padding(24.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
        verticalArrangement = Arrangement.Center
    ) {
        Text("你好，安卓！", fontSize = 28.sp)
        Spacer(Modifier.height(16.dp))
        Text("你点了 $count 次", fontSize = 18.sp)
        Spacer(Modifier.height(16.dp))
        Button(onClick = { count++ }) {
            Text("点我 +1")
        }
    }
}

@Composable
fun LoginScreen() {
    // 登录页占位：未登录时跳到这（以后做真实登录）
    Box(
        modifier = Modifier.fillMaxSize().background(Color.White),
        contentAlignment = Alignment.Center
    ) {
        Column(horizontalAlignment = Alignment.CenterHorizontally) {
            Text("请登录", fontSize = 24.sp, fontWeight = FontWeight.Bold)
            Spacer(Modifier.height(8.dp))
            Text("（占位页，以后接真实登录）", fontSize = 14.sp, color = Color.Gray)
        }
    }
}

@Preview(showBackground = true)
@Composable
fun FirstScreenPreview() {
    AndroidProTheme {
        FirstScreen()
    }
}
