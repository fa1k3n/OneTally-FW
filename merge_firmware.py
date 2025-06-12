import re

Import("env")
APP_BIN = "$BUILD_DIR/${PROGNAME}.bin"
BOARD_CONFIG = env.BoardConfig()

def merge_bin(source, target, env):

    srcDir = env.get("PROJECT_INCLUDE_DIR")
    version = "unknown"
    with open(srcDir + "/tally-firmware.hpp") as f:
        version = re.search("\"(\d+\.\d+\.\d+)\"", f.read()).group()

    flash_images = env.Flatten(env.get("FLASH_EXTRA_IMAGES", [])) + [ "0x187000", "$BUILD_DIR/${ESP32_FS_IMAGE_NAME}.bin" ] + ["$ESP32_APP_OFFSET", APP_BIN]

    env.Execute(
        " ".join(
            [
                "$PYTHONEXE",
                "$OBJCOPY",
                "--chip",
                BOARD_CONFIG.get("build.mcu", "esp32"),
                "merge_bin",
                "--fill-flash-size",
                BOARD_CONFIG.get("upload.flash_size", "4MB"),
                "-o",
                "$BUILD_DIR/OneTally-v" + version + ".bin"
            ]
            + flash_images
        )
    )

env.AddCustomTarget(
    name="build_bin",
    dependencies=None,
    actions=[
        "pio run -t buildfs",
        "pio run",
        merge_bin
    ],
    title="Build merged bin",
    description="Merge all different bins into one"
)