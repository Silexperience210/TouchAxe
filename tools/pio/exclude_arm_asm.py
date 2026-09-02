"""
LVGL 9.5 ships hand-written ARM Helium (MVE) assembly for its software blender.
Those .S files are guarded for ARM in C, but PlatformIO still hands them to the
Xtensa assembler, which chokes on the stdint.h typedefs they pull in:

    xtensa-esp32s3-elf/sys-include/stdint.h:51: Error: unknown opcode 'typedef'

The result is a build that fails on lv_blend_helium.S for reasons that have
nothing to do with this project. This middleware drops ARM-only assembly from
the build graph on non-ARM targets, and leaves everything else untouched.

Remove this file once upstream LVGL guards the sources at the build-system level.
"""

Import("env")  # noqa: F821

ARM_ONLY_MARKERS = ("helium", "neon")


def _is_arm_only_asm(path: str) -> bool:
    lowered = path.lower().replace("\\", "/")
    if not lowered.endswith(".s"):
        return False
    return any(marker in lowered for marker in ARM_ONLY_MARKERS)


def skip_arm_assembly(node):
    path = str(node)
    if _is_arm_only_asm(path):
        print("[lvgl] skipping ARM-only assembly on Xtensa: %s" % path.split("/")[-1])
        return None
    return node


mcu = env.BoardConfig().get("build.mcu", "")
if not mcu.startswith("arm") and "cortex" not in mcu:
    env.AddBuildMiddleware(skip_arm_assembly)
