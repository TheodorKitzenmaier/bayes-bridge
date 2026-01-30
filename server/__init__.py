import flask

from .api import api

# Create app
app = flask.Flask(__name__)
app.register_blueprint(api, url_prefix=f"/{api.name}")

def run():
    app.run()
