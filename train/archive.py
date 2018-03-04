
import random
import numpy as np
import struct

BOARD_SIZE = 19
BOARD_SQ = BOARD_SIZE*BOARD_SIZE
HISTORY_STEP = 8
INPUT_CHANNELS = 2*HISTORY_STEP + 2

RESIDUAL_FILTERS = 192
RESIDUAL_BLOCKS = 13



class GameRec(object):
    def __init__(self, data):
        self.raw = data
        self.steps = None

    def encode(self, index):
        if self.steps is None:
            self.parse()

        if index%2 == 0:
            player_is_black = True
            my_offset = 0
            opp_offset = HISTORY_STEP
        else:
            player_is_black = False
            my_offset = HISTORY_STEP
            opp_offset = 0

        blacks = [0]*BOARD_SQ
        whites = [0]*BOARD_SQ
        black_history = []
        white_history = []
        probs = []
        planes = [None]*(HISTORY_STEP*2)

        black_move = True
        for step in range(index):
            pos = self.steps[step][0]
            if pos < BOARD_SQ:
                if black_move:
                    blacks[pos] = 1
                else:
                    whites[pos] = 1

            # remove stones
            for rmpos in self.steps[step][1]:
                blacks[rmpos] = 0
                whites[rmpos] = 0

            h = index - step - 1
            if h < HISTORY_STEP:
                planes[h + my_offset] = np.array(blacks, dtype=np.uint8)
                planes[h + opp_offset] = np.array(whites, dtype=np.uint8)

            black_move = not black_move

        for i in range(HISTORY_STEP*2):
            if planes[i] is None:
                planes[i] = np.array([0]*BOARD_SQ, dtype=np.uint8)

        
        probs = self.steps[index][2]
        if len(probs) == 0:
            probs = [0] * (BOARD_SQ+1)
            probs[self.steps[index][0]] = 1

        probabilities = np.array(probs).astype(float)

        result = self.result
        if not player_is_black:
            result = -result

        return planes, probabilities, player_is_black, result

    def parse(self):
        if self.steps is not None:
            return

        steps = []
        valid_steps = []
        data = self.raw
        i = 0
        result,steps_count = struct.unpack('<bH', data[i:i+3])
        i = i+3

        if result not in (0, 1, -1):
            raise Exception("bad game result")


        for step in range(steps_count):
            move, = struct.unpack('<H', data[i:i+2])
            i = i+2
            pos = 0x1ff & move

            if pos > BOARD_SQ:
                raise Exception("invalid move pos %d" % pos)

            probs = []
            removes  = []
            # remove stones
            if move & 0x8000:
                rm, = struct.unpack('<H', data[i:i+2])
                i = i+2
                removes = struct.unpack('H'*rm, data[i:i+rm*2])
                i = i+rm*2
                for rmpos in removes:
                    if rmpos >= BOARD_SQ:
                        raise Exception("invalid remove pos %d" % rmpos)

            if move & 0x4000:
                probs = struct.unpack('f'*(BOARD_SQ+1), data[i:i+(BOARD_SQ+1)*4])
                i = i+(BOARD_SQ+1)*4

            if move & 0x2000:
                valid = False
            else:
                valid_steps.append(len(steps))

            steps.append((pos, removes, probs))



        self.steps = steps
        self.result = result
        self.valid_steps = valid_steps
        


class GameArchive(object):
    def __init__(self, path):

        self.games = []

        with open(path, mode='rb') as file:
            type = file.read(1)
            if type != b'G':
                raise Exception("bad archive type magic")
            bsize, = struct.unpack('B', file.read(1))
            if bsize != BOARD_SIZE:
                raise Exception("not expect traindata for BOARD_SIZE %d" % (BOARD_SIZE,))

            while True:
                try:
                    magic,size = struct.unpack('<ci', file.read(5))
                    if magic != b'g':
                        raise Exception("bad archive line magic")
                except struct.error:
                    break # EOF

                self.games.append(GameRec(file.read(size)))
                if len(self.games)%10000 == 0:
                    print('parsed ', len(self.games))





if __name__ == "__main__":
    obj = GameArchive('../../data/test1.data')
    planes, probabilities, player_is_black, result = obj.games[0].encode(7)
    print(planes[0], player_is_black)

