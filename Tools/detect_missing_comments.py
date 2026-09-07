# Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential.
"""Compatibility command: shared comment checker in missing-only mode."""
import sys
from check_comments import main

if __name__ == "__main__":
    sys.exit(main(missing_only=True))
