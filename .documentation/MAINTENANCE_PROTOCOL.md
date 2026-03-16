# AI Maintenance Protocol

## Prime Directive
**NO TASK IS COMPLETE UNTIL THE DOCUMENTATION REFLECTS THE CHANGE.**

## Trigger Actions
| If you do this...     | You MUST update this...                        |
| :-------------------- | :--------------------------------------------- |
| **Add/Rename Pass**   | `.ai/ARCHITECTURE.md` (Update Pipeline)        |
| **Modify Build/Libs** | `.ai/TECH_STACK.md`                            |
| **Create File**       | `.ai/PROJECT_MAP.md`                           |
| **Added License**     | Do not add Licenses! Remove them from the file |

## Verification
End every coding response with a checklist confirming you updated these files.

## Testing Standards
Required after every implementation task:
1. **Build LLTA**: Run `./config.sh config` and `./config.sh build`
2. **Run Regression Test**: Execute `python3 tests/regression_test.py`
3. **Verify Result**:
   - **GREEN**: Ideal. Matches baseline.
   - **YELLOW**: Acceptable status. Make sure we know why the computed WCET differs.
   - **RED**: **FAILURE**. Do not proceed/submit. Fix the crash or unbounded issue.
