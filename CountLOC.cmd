@ECHO OFF
CLOC PowerSolutions.PowerFlow PowerSolutions.Interop PowerFlowSolver --exclude-dir=Library,"My Project",obj,bin  --exclude-lang=ASP.Net,"MSBuild script"  --out=LOC.txt
TYPE LOC.txt
PAUSE