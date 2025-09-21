python -m pip install glad --break-system-packages
# todo: check current path to know if to traverse ../ or not
python -m glad --profile=core --api=gl=4.6 --generator=c --out-path=modules/glad
