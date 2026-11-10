pluginManagement {
    repositories {
        google {
            content {
                includeGroupByRegex("com\\.android.*")
                includeGroupByRegex("com\\.google.*")
                includeGroupByRegex("androidx.*")
            }
        }
        mavenCentral()
        gradlePluginPortal()
    }
}
plugins {
    id("org.gradle.toolchains.foojay-resolver-convention") version "1.0.0"
}
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        // Distributable AAR also published here by CreateAAR.
        maven {
            url = uri("${rootDir}/../AndroidSDK/maven")
        }
    }
}

rootProject.name = "androidPro"
include(":app")

// Dev/IDE: same sources that CreateAAR packages into AndroidSDK/core.aar
include(":coresdk")
project(":coresdk").projectDir =
    file("../../../app/Android/CreateAAR/project/core")
