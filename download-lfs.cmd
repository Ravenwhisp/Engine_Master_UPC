@echo off
git lfs install
git lfs pull
git lfs fetch --all
git lfs checkout
pause