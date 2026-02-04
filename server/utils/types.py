import subprocess

from typing import Optional

WORKER_FILE_PATH = "/tmp/bayes"

class ModelWorker:
    id: str
    input_file: str
    prior_file: str
    output_file: str
    derived_file: str
    process: Optional[subprocess.Popen]

    def __init__(self, id, input_file, prior_file, output_file, derived_file):
        self.id = id
        self.input_file = input_file
        self.prior_file = prior_file
        self.output_file = output_file
        self.derived_file = derived_file
        self.process = None
