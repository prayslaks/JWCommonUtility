# Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT
"""Compatibility command: shared comment checker in missing-only mode."""
import sys
from check_comments import main

if __name__ == "__main__":
    sys.exit(main(missing_only=True))
