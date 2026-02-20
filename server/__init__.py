import flask
import pathlib

from .api import api
from .utils.types import WORKER_FILE_PATH

# Create app
app = flask.Flask(__name__)
app.register_blueprint(api, url_prefix=f"/{api.name}")

def run():
    pathlib.Path(WORKER_FILE_PATH).mkdir(mode=777,parents=True,exist_ok=True)
    app.run()
