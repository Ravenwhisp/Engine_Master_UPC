#!/usr/bin/env python3
"""
Migration script: Convert std::string prefab-path fields to AssetRef<Prefab> in .scene files.

Reads a .scene JSON file, finds ScriptComponent fields that were previously stored
as raw file-path strings (e.g. "Assets/Prefabs/Arrow.prefab"), resolves them to
AssetReference objects (UID + MD5 content hash + type) by reading the corresponding
.metadata files, and rewrites the scene file.

Usage:
    python migrate_prefab_paths_to_assetref.py path/to/scene.scene [--dry-run]

The --dry-run flag prints the changes without modifying the file.

Field name mapping:
    m_arrowPrefabPath          -> m_arrowPrefab
    m_barrierPrefabPath        -> m_barrierPrefab
    m_healthPrefabPath         -> m_healthPrefab
    m_collectParticlePrefabPath -> m_collectParticlePrefab
    m_particlePrefabPath       -> m_particlePrefab
    m_healthPickupPrefabPath   -> m_healthPickupPrefab
    m_explosionPrefab          (hardcoded path -> AssetRef field)
    m_dustPrefab               (hardcoded path -> AssetRef field)
"""

import json
import os
import sys
import hashlib
from pathlib import Path

# Mapping: old string field name -> new AssetRef field name
FIELD_RENAME_MAP = {
    "m_arrowPrefabPath": "m_arrowPrefab",
    "m_barrierPrefabPath": "m_barrierPrefab",
    "m_healthPrefabPath": "m_healthPrefab",
    "m_collectParticlePrefabPath": "m_collectParticlePrefab",
    "m_particlePrefabPath": "m_particlePrefab",
    "m_healthPickupPrefabPath": "m_healthPickupPrefab",
    "m_enemyPrefabPaths": None,  # handled separately
}


def compute_md5_hex(file_path: str) -> str:
    """Compute MD5 hex digest of a file."""
    h = hashlib.md5()
    with open(file_path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def read_metadata(asset_path: str) -> dict | None:
    """
    Read the .metadata file for a given asset path.
    Returns a dict with 'uid', 'contentHash', 'type' or None if not found.
    """
    meta_path = asset_path + ".metadata"
    if not os.path.isfile(meta_path):
        print(f"  WARNING: Metadata not found: {meta_path}")
        return None

    try:
        with open(meta_path, "r", encoding="utf-8") as f:
            meta = json.load(f)
        return meta
    except (json.JSONDecodeError, OSError) as e:
        print(f"  WARNING: Failed to read metadata {meta_path}: {e}")
        return None


def path_to_asset_ref(asset_path: str, project_root: str) -> dict | None:
    """
    Convert an asset file path to an AssetReference JSON object.
    Reads the .metadata file to get UID and content hash.
    """
    full_path = os.path.join(project_root, asset_path)

    meta = read_metadata(full_path)
    if meta is None:
        print(f"  INFO: Computing MD5 from source file as fallback...")
        if os.path.isfile(full_path):
            content_hash = compute_md5_hex(full_path)
            return {
                "uid": 0,  # unknown UID
                "libId": content_hash,
                "type": "PREFAB"
            }
        else:
            print(f"  WARNING: Asset file not found: {full_path}")
            return None

    return {
        "uid": meta.get("uid", 0),
        "libId": meta.get("contentHash", ""),
        "type": "PREFAB"
    }


def migrate_scene(input_path: str, dry_run: bool = False):
    """Main migration function."""
    scene_path = Path(input_path).resolve()
    project_root = scene_path.parent  # scene is in Assets/Scenes/, root is project dir
    # Navigate up to project root (Assets/Scenes/ -> ../..)
    while project_root.name.lower() != "assets" and project_root != project_root.parent:
        project_root = project_root.parent
    project_root = project_root.parent  # go up once more past Assets/

    print(f"Project root: {project_root}")
    print(f"Scene file:   {scene_path}")
    print()

    with open(scene_path, "r", encoding="utf-8") as f:
        data = json.load(f)

    changes = 0

    # Iterate all GameObjects in the scene
    game_objects = data.get("GameObjects", [])
    if isinstance(game_objects, list):
        for go in game_objects:
            changes += migrate_game_object(go, project_root)

    if changes == 0:
        print("No changes needed.")
        return

    print(f"\nTotal changes: {changes}")

    if dry_run:
        print("[DRY RUN] No file written.")
    else:
        backup_path = str(scene_path) + ".backup"
        os.replace(str(scene_path), backup_path)
        print(f"Backup saved to: {backup_path}")

        with open(scene_path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2)
        print(f"Scene saved to: {scene_path}")


def migrate_game_object(go: dict, project_root: str) -> int:
    """Migrate a single GameObject's script fields. Returns number of changes."""
    changes = 0

    # Components are stored as Component_0, Component_1, etc.
    for key, value in go.items():
        if not key.startswith("Component_"):
            continue

        comp_type = value.get("ComponentType")
        if comp_type != 18:  # ComponentType::SCRIPT = 18
            continue

        script_name = value.get("ScriptName", "")
        if not script_name:
            continue

        for field_name, new_field_name in FIELD_RENAME_MAP.items():
            if field_name not in value:
                continue

            old_value = value[field_name]

            # Skip if already an object (already migrated)
            if isinstance(old_value, dict) and "libId" in old_value:
                continue

            # Skip if not a string (shouldn't happen but be safe)
            if not isinstance(old_value, str):
                continue

            # Skip empty strings
            if not old_value.strip():
                continue

            print(f"  [{script_name}] {field_name} = \"{old_value}\"")

            asset_ref = path_to_asset_ref(old_value, project_root)

            if asset_ref and new_field_name:
                # Remove old field, add new field
                del value[field_name]
                value[new_field_name] = asset_ref
                print(f"    -> {new_field_name} = {asset_ref}")
                changes += 1
            elif asset_ref is None:
                print(f"    -> SKIPPED (could not resolve asset reference)")

    return changes


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)

    scene_file = sys.argv[1]
    is_dry_run = "--dry-run" in sys.argv

    if not scene_file.endswith(".scene"):
        print("ERROR: Expected a .scene file.")
        sys.exit(1)

    if not os.path.isfile(scene_file):
        print(f"ERROR: File not found: {scene_file}")
        sys.exit(1)

    migrate_scene(scene_file, dry_run=is_dry_run)
