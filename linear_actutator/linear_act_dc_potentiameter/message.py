from typing import List, Optional

from pydantic import BaseModel, Field


class Message(BaseModel):
    sensor_values: List[float] = Field(..., min_length=4, max_length=4)
    lin_acts: List[float] = Field(..., min_length=4, max_length=4)
    id: int


class ErrorMessage(BaseModel):
    id: int
    error: str
    details: Optional[str] = None
