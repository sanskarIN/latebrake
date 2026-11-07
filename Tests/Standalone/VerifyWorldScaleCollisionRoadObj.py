"""Independent strict numeric contract for the compiled physical-road OBJ.

The C++ producer builds the candidate mesh; this verifier reconstructs all
1024 centreline samples and all 8192 face indices independently in Python.
It proves source geometry only, not Unreal cooking or Chaos wheel contacts.
"""
import math
from pathlib import Path
import subprocess
import sys
import tempfile

from VerifyRoundedSmoothRoadObj import centerline_at, LAP, SECTIONS

VERTICES = 4 * SECTIONS
FACES = 8 * SECTIONS
UVS = 4 * (SECTIONS + 1)
PHYSICAL_HALF_WIDTH = 450.0
TOP = -1.0
BOTTOM = -43.0
UV_REPEAT_CM = 400.0


def require_close(actual, expect, label, tolerance=0.000002):
    if not math.isfinite(actual) or abs(actual - expect) > tolerance:
        raise ValueError(f"{label} out of bounds: {actual!r} vs {expect!r}")


def expected_vertex(index):
    section, corner = divmod(index, 4)
    world_distance = 10.0 * LAP * section / SECTIONS
    cx, cy, yaw = centerline_at(world_distance, world_scale=10.0)
    radians = math.radians(yaw)
    side = 1 if corner % 2 == 0 else -1
    return (
        cx - math.sin(radians) * side * PHYSICAL_HALF_WIDTH,
        cy + math.cos(radians) * side * PHYSICAL_HALF_WIDTH,
        TOP if corner < 2 else BOTTOM,
    )


def expected_triangle(group, section, half):
    first = 4 * section
    next_station = 4 * ((section + 1) % SECTIONS)
    lt, rt, lb, rb = first, first + 1, first + 2, first + 3
    nlt, nrt, nlb, nrb = (next_station, next_station + 1,
                          next_station + 2, next_station + 3)
    return (
        ((lt, rt, nrt), (lt, nrt, nlt)),
        ((lb, nrb, rb), (lb, nlb, nrb)),
        ((lt, nlt, nlb), (lt, nlb, lb)),
        ((rt, rb, nrb), (rt, nrb, nrt)),
    )[group][half]


def verify_obj(obj_path, mtl_path):
    obj_path, mtl_path = Path(obj_path), Path(mtl_path)
    if not obj_path.is_file() or not mtl_path.is_file():
        raise ValueError("Missing original compiled physical OBJ or MTL source")
    groups = [
        ("PhysicalRoadTop", "LatebrakePhysicalRoadTop"),
        ("PhysicalRoadBottom", "LatebrakePhysicalRoadSides"),
        ("PhysicalRoadLeftWall", "LatebrakePhysicalRoadSides"),
        ("PhysicalRoadRightWall", "LatebrakePhysicalRoadSides"),
    ]
    vertices = uv_count = face_count = 0
    group = -1
    group_faces = 0
    selected_material = None
    library_seen = object_seen = False
    seam_uvs = set()
    for line in obj_path.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        parts = line.split()
        if parts[0] == "mtllib":
            if library_seen or line != f"mtllib {mtl_path.name}":
                raise ValueError("Unexpected or repeated material-library path")
            library_seen = True
        elif parts[0] == "o":
            if object_seen or line != "o LatebrakeWorldScaleContinuousPhysicalRoad":
                raise ValueError("Unexpected or repeated physical road object name")
            object_seen = True
        elif parts[0] == "v":
            if not object_seen or vertices >= VERTICES or len(parts) != 4:
                raise ValueError("Unexpected original collision vertex count")
            for axis, (x, expect) in enumerate(zip(parts[1:], expected_vertex(vertices))):
                require_close(float(x), expect, f"vertex {vertices} coordinate {axis}")
            vertices += 1
        elif parts[0] == "vt":
            if vertices != VERTICES or uv_count >= UVS or len(parts) != 3:
                raise ValueError("Unexpected physical road UV")
            station, corner = divmod(uv_count, 4)
            require_close(float(parts[1]), 10 * LAP * station /
                          (SECTIONS * UV_REPEAT_CM), "longitudinal road UV")
            require_close(float(parts[2]), 1 if corner % 2 == 0 else 0,
                          "lateral road UV")
            uv_count += 1
        elif parts[0] == "g":
            if vertices != VERTICES or uv_count != UVS or len(parts) != 2 or (
                group >= 0 and group_faces != 2 * SECTIONS
            ):
                raise ValueError("Incorrect physical surface group ordering")
            group += 1
            if group >= 4 or parts[1] != groups[group][0]:
                raise ValueError("Unrecognized or reordered physical surface group")
            group_faces = 0
            selected_material = None
        elif parts[0] == "usemtl":
            if group < 0 or selected_material is not None or len(parts) != 2 or (
                parts[1] != groups[group][1]
            ):
                raise ValueError("Incorrect physical road materials")
            selected_material = parts[1]
        elif parts[0] == "f":
            if (selected_material is None or group not in range(4) or
                group_faces >= 2 * SECTIONS or len(parts) != 4):
                raise ValueError("Wrong triangle group or face count")
            ids = []
            for field in parts[1:]:
                pair = field.split("/")
                if len(pair) != 2 or not all(x.isascii() and x.isdigit() for x in pair):
                    raise ValueError("Noncanonical physical road triangle")
                vertex, uv = (int(x) for x in pair)
                if not 1 <= vertex <= VERTICES or not 1 <= uv <= UVS:
                    raise ValueError("Out-of-range road face/UV reference")
                ids.append(vertex - 1)
                expect_uv = vertex
                if group_faces // 2 == SECTIONS - 1 and (vertex - 1) // 4 == 0:
                    expect_uv = 4 * SECTIONS + (vertex - 1) % 4 + 1
                if uv != expect_uv:
                    raise ValueError("Incorrect welded-lap UV reference")
                if uv > VERTICES:
                    seam_uvs.add(uv)
            expected = expected_triangle(group, group_faces // 2, group_faces % 2)
            if tuple(ids) != expected:
                raise ValueError("Incorrectly wound or disconnected physical triangle")
            group_faces += 1
            face_count += 1
        else:
            raise ValueError(f"Unrecognized physical OBJ record: {parts[0]}")
    if (not library_seen or not object_seen or vertices != VERTICES or
        uv_count != UVS or face_count != FACES or group != 3 or
        group_faces != 2 * SECTIONS or seam_uvs != set(range(4097, 4101))):
        raise ValueError("Incomplete full-width closed physical road OBJ")
    material = mtl_path.read_text(encoding="utf-8")
    if (material.count("newmtl LatebrakePhysicalRoadTop") != 1 or
        material.count("newmtl LatebrakePhysicalRoadSides") != 1 or
        "map_" in material):
        raise ValueError("Unexpected physical-road MTL or external texture")
    return vertices, face_count


def main():
    if len(sys.argv) != 2:
        raise SystemExit("Usage: VerifyWorldScaleCollisionRoadObj.py <compiled-exporter>")
    with tempfile.TemporaryDirectory(prefix="latebrake-physical-road-") as path:
        destination = Path(path) / "WorldScaleContinuousPhysicalRoad.obj"
        run = subprocess.run([sys.argv[1], str(destination)],
                             capture_output=True, text=True, timeout=40, check=True)
        if ("collision=MODEL_NOT_COOKED" not in run.stdout or
            "NOT_DRIVEN" not in run.stdout or run.stderr):
            raise ValueError("Physical exporter lacks unverified collision warning")
        v, f = verify_obj(destination, destination.with_suffix(".mtl"))
    print(f"Verified original continuous 1.7-km full-width physical road: "
          f"{v} vertices, {f} wound triangles, 9m x 42cm, seam and UV closure. "
          "NO UNREAL/CHAOS RUNTIME TEST PERFORMED.")


if __name__ == "__main__":
    main()
