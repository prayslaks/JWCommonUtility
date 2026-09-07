# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.
"""Compatibility entry point for the shared structure scanner."""
from pathlib import Path
import sys
sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "Tools"))
from scan_structure import main

if __name__ == "__main__":
    try:
        sys.exit(main())
    except OSError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        sys.exit(2)
