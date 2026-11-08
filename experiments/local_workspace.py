"""Resolve private conversion inputs/outputs outside the public source tree."""
import os
from pathlib import Path

def local_workspace():
    value = os.environ.get('RDR2PAL_LOCAL_WORKSPACE')
    if not value:
        raise RuntimeError('Set RDR2PAL_LOCAL_WORKSPACE to your private development workspace.')
    workspace = Path(value).resolve()
    repository = Path(__file__).resolve().parents[1]
    if workspace == repository or repository in workspace.parents:
        raise RuntimeError('The private workspace must be outside this source repository.')
    return workspace
