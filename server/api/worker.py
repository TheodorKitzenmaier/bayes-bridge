import flask
import flask_restx

worker_namespace = flask_restx.Namespace(
    "worker",
    "Endpoints to start and monitor model workers."
)

@worker_namespace.route("/start")
class StartWorker(flask_restx.Resource):
    def get(self):
        return {"test": True}
