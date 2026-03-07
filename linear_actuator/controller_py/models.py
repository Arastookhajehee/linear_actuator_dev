from pydantic import BaseModel
import json
import threading

class actuator(BaseModel):

    def __init__(self, lock, id, current, target):
        self.lock = lock
        self.id = current
        self.target = target
        
    def update_current(value):
        with lock():
            self.value = value


    def update_target(value):
        with lock():
            self.target = value

    def to_json():
        return json.dumps(self.model())
    
    def from_json(json):
        actuator.model_validate_json(json)
        

