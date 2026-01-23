import logging
import flask
import flask_restx

from .worker import worker_namespace

# Set up logging.
api_logger = logging.getLogger("api")

# Set up API.
api = flask.Blueprint("bayes_bridge_api", __name__)
api_v1 = flask_restx.Api(api)

api_v1.add_namespace(worker_namespace, "/worker")
