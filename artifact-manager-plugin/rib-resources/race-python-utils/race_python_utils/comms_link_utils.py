"""
    Purpose:
        Utilities for generating Comms Links
"""

# Python Library Imports
from typing import Any, Dict, List, Optional


###
# Generate Link Functions
###


def generate_link_properties_dict(
    link_type: str,
    transmission_type: str,
    reliability: bool,
    exp_latency_ms: str,
    exp_bandwidth_bps: str,
    exp_variance_pct: float = 0.1,
    supported_hints: List[str] = [],
) -> Dict[str, Any]:
    """
    Purpose:
        Generate RACE Link Properties Dict from values.

        Expected will be passed in with a variance to build best/worst/expected
    Args:
        link_type: ("send" or "receive") indicating the type of link
        transmission_type: ("unicast" or "multicast") indicating the
            transmission type
        reliability: indicates if the link is reliable to Network Manager
        exp_latency_ms: expected latency using the link in milliseconds
        exp_bandwidth_bps: expected bandwidth using the link in bytes
        exp_variance_pct: expected variance (as a percentage between 0.0 and 1.0) between
            best and worst case scenarios.
        supported_hints: all supported hints for the link
    Return:
        link_properties: Dict containing link properties
    """

    return {
        "type": link_type,
        "reliable": reliability,
        transmission_type: True,  # set the transmission type key
        "supported_hints": supported_hints,
        "expected": {
            link_type: {
                "latency_ms": exp_latency_ms,
                "bandwidth_bps": exp_bandwidth_bps,
            }
        },
        "best": {
            link_type: {
                "latency_ms": int(exp_latency_ms * (1 - exp_variance_pct)),
                "bandwidth_bps": int(exp_bandwidth_bps * (1 + exp_variance_pct)),
            }
        },
        "worst": {
            link_type: {
                "latency_ms": int(exp_latency_ms * (1 + exp_variance_pct)),
                "bandwidth_bps": int(exp_bandwidth_bps * (1 - exp_variance_pct)),
            }
        },
    }
