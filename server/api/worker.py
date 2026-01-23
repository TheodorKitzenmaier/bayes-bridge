import flask
import flask_restx

worker_namespace = flask_restx.Namespace(
	"worker",
	"Endpoints to start and monitor model workers."
)
