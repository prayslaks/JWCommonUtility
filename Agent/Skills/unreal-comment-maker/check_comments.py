# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
"""Compatibility entry point for the shared JWCommonUtility comment checker."""

from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[3] / "Tools"))
from check_comments import main


if __name__ == "__main__":
    sys.exit(main())
