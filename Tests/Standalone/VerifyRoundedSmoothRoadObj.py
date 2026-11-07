"""Independently inspect the actual compiled OBJ/MTL original road preview.

This runs the C++ executable. A valid process exit alone is not a valid 3D
mesh: check vertices, UV lap seam, face indices and all 1024 roadway stations.
No licensed Unreal Engine or fabricated .uasset is involved.
"""
import math
from pathlib import Path
import subprocess
import sys
import tempfile

SECTIONS = 1024
VERTICES = 4096
TRIANGLES = 8192
UVS = 4100
WIDTH = 425.0
TOP = -0.55
BOTTOM = -1.0
RADIUS = 800.0
PI = math.pi
QUARTER = PI / 2 * RADIUS
LENGTH_H = 4000.0
LENGTH_V = 2000.0
LAP = 2 * LENGTH_H + 2 * LENGTH_V + 4 * QUARTER


def centerline_at(s):
    s = math.fmod(s, LAP)
    for section in range(8):
        if section % 2 == 0:
            straight = section // 2
            length = LENGTH_H if straight % 2 == 0 else LENGTH_V
            if s < length:
                return (
                    (-2000.0 + s, -1800.0, 0.0),
                    (2800.0, -1000.0 + s, 90.0),
                    (2000.0 - s, 1800.0, 180.0),
                    (-2800.0, 1000.0 - s, 270.0),
                )[straight]
            s -= length
        else:
            corner = section // 2
            if s < QUARTER or section == 7:
                angle = -PI / 2 + corner * PI / 2 + s / RADIUS
                cx, cy = ((2000.0, -1000.0), (2000.0, 1000.0),
                          (-2000.0, 1000.0), (-2000.0, -1000.0))[corner]
                return (cx + RADIUS * math.cos(angle),
                        cy + RADIUS * math.sin(angle),
                        (angle + PI / 2) * 180 / PI)
            s -= QUARTER
    raise ValueError("invalid forward lap station")


def expected_vertex(vertex):
    section, corner = divmod(vertex, 4)
    x, y, deg = centerline_at(LAP * section / SECTIONS)
    angle = math.radians(deg)
    side = 1 if corner % 2 == 0 else -1
    return (x - math.sin(angle) * WIDTH * side,
            y + math.cos(angle) * WIDTH * side,
            TOP if corner < 2 else BOTTOM)


def require_close(a, b, label, tol=0.000002):
    if not math.isfinite(a) or abs(a - b) > tol:
        raise ValueError(f"Invalid original road OBJ {label}: {a!r} vs {b!r}")


def verify_obj(obj, material):
    if not obj.is_file() or not material.is_file():
        raise ValueError("Compiled C++ visual road and MTL must both exist")
    lines = obj.read_text(encoding="utf-8").splitlines()
    face_count = 0
    vertex_count = 0
    uv_count = 0
    material_groups = []
    saw_mtllib = False
    seam_uvs = set()
    for line in lines:
        if line.startswith("mtllib "):
            if line != f"mtllib {material.name}" or saw_mtllib:
                raise ValueError("Unexpected or duplicate OBJ material library path")
            saw_mtllib = True
        elif line.startswith("v "):
            if vertex_count >= VERTICES:
                raise ValueError("Extra visual road vertices")
            values = line.split()
            if len(values) != 4:
                raise ValueError("Incomplete OBJ world-space vertex")
            for axis, (actual, expect) in enumerate(
                zip((float(x) for x in values[1:]), expected_vertex(vertex_count))
            ):
                require_close(actual, expect, f"vertex {vertex_count} axis {axis}")
            vertex_count += 1
        elif line.startswith("vt "):
            if uv_count >= UVS:
                raise ValueError("Extra UV stations")
            values = line.split()
            if len(values) != 3:
                raise ValueError("Incomplete OBJ UV")
            section, edge = divmod(uv_count, 4)
            require_close(float(values[1]), LAP * section / SECTIONS / 400.0,
                          f"UV {uv_count} longitudinal")
            require_close(float(values[2]), 1.0 if edge % 2 == 0 else 0.0,
                          f"UV {uv_count} lateral")
            uv_count += 1
        elif line.startswith("g "):
            material_groups.append((line[2:], 0))
        elif line.startswith("f "):
            if not material_groups or face_count >= TRIANGLES:
                raise ValueError("Unexpected OBJ surface group or face count")
            fields = line.split()
            if len(fields) != 4:
                raise ValueError("All OBJ faces must be triangles")
            ids = []
            for field in fields[1:]:
                values = field.split("/")
                if len(values) != 2 or not all(v.isdigit() for v in values):
                    raise ValueError("Invalid OBJ vertex/texture link")
                vi, ui = map(int, values)
                if not 1 <= vi <= VERTICES or not 1 <= ui <= UVS:
                    raise ValueError("Out-of-range OBJ face or UV index")
                ids.append(vi)
                if ui > VERTICES:
                    seam_uvs.add(ui)
            if len(set(ids)) != 3:
                raise ValueError("Degenerate OBJ triangle")
            name, count = material_groups[-1]
            material_groups[-1] = (name, count + 1)
            face_count += 1

    expected_groups = [
        ("RoadTop", 2048), ("RoadUnderside", 2048),
        ("RoadLeftShoulder", 2048), ("RoadRightShoulder", 2048),
    ]
    if not saw_mtllib or vertex_count != VERTICES or uv_count != UVS or \
       face_count != TRIANGLES or material_groups != expected_groups:
        raise ValueError("Incomplete or malformed continuous-road OBJ layout")
    if seam_uvs != {4097, 4098, 4099, 4100}:
        raise ValueError("UV lap seam is stretched or incorrectly wrapped")
    mtl = material.read_text(encoding="utf-8")
    if "newmtl LatebrakeAsphalt" not in mtl or \
       "newmtl LatebrakeRoadSides" not in mtl:
        raise ValueError("Original surface and shoulder material placeholders missing")
    return vertex_count, face_count


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: VerifyRoundedSmoothRoadObj.py <compiled-exporter>")
    with tempfile.TemporaryDirectory(prefix="latebrake-original-road-") as directory:
        destination = Path(directory) / "RoundedSmoothPreview.obj"
        result = subprocess.run(
            [sys.argv[1], str(destination)],
            check=True, timeout=30, capture_output=True, text=True,
        )
        if "collision=off" not in result.stdout or result.stderr:
            raise ValueError("Compiled original OBJ exporter log has invalid provenance")
        vertices, faces = verify_obj(destination, destination.with_suffix(".mtl"))
    print(f"Verified {vertices} continuous road OBJ vertices, {faces} triangles, UV closure, and original MTL against an independent centerline")


if __name__ == "__main__":
    main()
