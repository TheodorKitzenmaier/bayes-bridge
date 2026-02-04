import flask
import flask_restx
import random
import string
import pathlib
import subprocess

from ..utils.types import ModelWorker, WORKER_FILE_PATH

worker_namespace = flask_restx.Namespace(
    "worker",
    "Endpoints to start and monitor model workers."
)

workers: dict[str, ModelWorker] = {}

@worker_namespace.route("/init")
class InitWorker(flask_restx.Resource):
    def post(self):
        # Create a worker.
        worker_id = "".join(random.choice(string.ascii_lowercase) for _ in range(16))
        while worker_id in workers.keys():
            worker_id = "".join(random.choice(string.ascii_lowercase) for _ in range(16))

        # Caller passes in file name for input.
        data = flask.request.json
        input_file = data.get("input_file", None)
        if not input_file or input_file == "" or input_file[0] != "/":
            input_file = f"{WORKER_FILE_PATH}/{worker_id}.in"

        worker = ModelWorker(worker_id, input_file, f"{WORKER_FILE_PATH}/{worker_id}.priors", f"{WORKER_FILE_PATH}/{worker_id}.out", f"{WORKER_FILE_PATH}/{worker_id}.drv")
        workers[worker_id] = worker

        # Return a handle for the worker and file names.
        return flask.make_response(f"Worker_ID: {worker.id}\nInput_Path: {worker.input_file}\nPrior_Path: {worker.prior_file}\nOutput_Path: {worker.output_file}\nDerived_Path: {worker.derived_file}")

@worker_namespace.route("/start")
class StartWorker(flask_restx.Resource):
    def post(self):
        # Caller passes in worker id and command.
        data = flask.request.json
        worker_id = data.get("worker_id", None)
        command:str = data.get("command", None)
        if None in [worker_id, command]:
            return flask.make_response("Error: Must supply \"worker_id\" and \"command\".", 400)
        worker = workers.get(worker_id, None)
        if not worker:
            return flask.make_response(f"Error: No worker found with id {worker_id}.", 404)

        try:
            # Check for files.
            input = pathlib.Path(worker.input_file)
            priors = pathlib.Path(worker.prior_file)
            assert input.exists(), f"Failed to find input file {input}."
            assert input.is_file(), f"Input file {input} is not a file."
            assert priors.exists(), f"Failed to find priors file {priors}."
            assert priors.is_file(), f"Priors file {priors} is not a file."

            # Check the worker is not already running.
            assert worker.process is None, f"Worker is already running."
        except AssertionError as e:
            return flask.make_response(f"Error: {str(e)}", 400)

        # Format command.
        command = command.replace("{input_file}", worker.input_file).replace("{prior_file}", worker.prior_file).replace("{output_file}", worker.output_file)

        # Run worker.
        worker.process = subprocess.Popen(command, shell=True)

        return command

@worker_namespace.route("/query")
class QueryWorker(flask_restx.Resource):
    def get(self):
        # Caller passes in worker id.
        worker_id = flask.request.args.get("worker_id", None)
        if not worker_id:
            return flask.make_response("Error: Must supply \"worker_id\".", 400)
        worker = workers.get(worker_id, None)
        if not worker:
            return flask.make_response(f"Error: No worker found with id {worker_id}.", 404)

        # Return status of worker.
        if not worker.process:
            return flask.make_response("Ready")
        elif not worker.process.poll():
            return flask.make_response("Running")
        else:
            return flask.make_response(f"Done: {worker.process.returncode}")

@worker_namespace.route("/collect")
class CollectWorker(flask_restx.Resource):
    def post(self):
        # Caller passes in worker id and command.
        data = flask.request.json
        worker_id = data.get("worker_id", None)
        if not worker_id:
            return flask.make_response("Error: Must supply \"worker_id\".", 400)

        # Remove worker.
        worker = workers.pop(worker_id, None)
        if not worker:
            return flask.make_response(f"Error: No worker found with id {worker_id}.", 404)
        return flask.make_response("Worker Collected")
