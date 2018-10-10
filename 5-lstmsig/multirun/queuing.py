from azure.servicebus import ServiceBusService, Message, Queue
import itertools, tabulate

primaryKey="SOME_PRIMARY_KEY"

bus_service=ServiceBusService(
    service_namespace="jezsbus",
    shared_access_key_name="RootManageSharedAccessKey", 
    shared_access_key_value=primaryKey)

_name='taskqueue'

def queue_length():
    return bus_service.get_queue(queue_name=_name).message_count

def empty_queue():
    while 1:
        msg=bus_service.receive_queue_message(queue_name=_name, peek_lock=False, timeout=0)
        if msg.body is None:
            if 0 == queue_length():
                return
            raise RuntimeError("failed to empty queue")
def create_queue():
    bus_service.create_queue(_name)

#fill_queue and get_from_queue require something like a ParamSet:
#must provide from_string_index and size.
class ParameterSet:
    def __init__(self, possibilities):
        self.possibilities=possibilities
        self.all_combinations=[list(i) for i in itertools.product(*possibilities)]
        self.size = len(self.all_combinations)
    def from_int_index(self,i):
        return self.all_combinations[i]
    def from_string_index(self,i):
        return self.from_int_index(int(i))
class ExplicitParameterSet:
    def __init__(self, wanted_combinations):
        self.wanted_combinations = wanted_combinations
        self.size = len(self.wanted_combinations)
    def from_int_index(self,i):
        return self.wanted_combinations[i]
    def from_string_index(self,i):
        return self.from_int_index(int(i))

def _fill_queue(maximum):
    for i in range(maximum):
        bus_service.send_queue_message(_name, Message(str(i)))

def fill_queue(parameters):
    empty_queue()
    _fill_queue(parameters.size)

#returns None when nothing to do
def get_from_queue(parameters):
    msg = bus_service.receive_queue_message(queue_name=_name, peek_lock=False, timeout=0)
    if msg.body is None:
        return None
    return parameters.from_string_index(msg.body)
