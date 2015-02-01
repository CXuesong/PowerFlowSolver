@ECHO OFF
CLOC PowerFlowSolver PowerFlowSolver.Interop PowerFlowSolver.GridRouter GridGenerator GridDrawing --exclude-dir=Library,"My Project",obj,bin  --exclude-lang=ASP.Net,"MSBuild script"
PAUSE