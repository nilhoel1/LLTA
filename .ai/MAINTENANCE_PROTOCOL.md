# AI Maintenance Protocol

## Prime Directive
**NO TASK IS COMPLETE UNTIL THE DOCUMENTATION REFLECTS THE CHANGE.**

## Trigger Actions
| If you do this...         | You MUST update this...                 |
| :------------------------ | :-------------------------------------- |
| **Add/Rename Pass**       | `.ai/ARCHITECTURE.md` (Update Pipeline) |
| **Modify Build/Libs**     | `.ai/TECH_STACK.md`                     |
| **Create File**           | `.ai/PROJECT_MAP.md`                    |
| **Added License to File** | Remove it! Do not ad Licenses to files. |

## Verification
End every coding response with a checklist confirming you updated these files.

## Testing Standards
Required after every implementation task:
1. **Run Regression Test**: Execute `python3 tests/regression_test.py`
2. **Verify Result**:
   - **GREEN**: Ideal. Matches baseline.
   - **YELLOW**: Acceptable status. Ensure WCET changes are justified.
   - **RED**: **FAILURE**. Do not proceed/submit. Fix the crash or unbounded issue.
