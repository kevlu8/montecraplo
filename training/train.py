import torch
import torch.nn as nn
import torch.nn.functional as F

L1_SIZE = 128
L2_SIZE = 256

class MCTSNet(nn.Module):
	def __init__(self):
		super(MCTSNet, self).__init__()
		
		self.fc1 = nn.Linear(768, L1_SIZE)
		self.fc2 = nn.Linear(L1_SIZE * 2, L2_SIZE)

		self.policy_head = nn.Linear(L2_SIZE, 4096 + 72) # 72 for underpromotions (24 moves, 3 other pieces) - technically only 66, but this makes indexing easier
		self.value_head = nn.Linear(L2_SIZE, 1)

	def forward(self, x):
		x = F.relu(self.fc1(x))
		x = F.relu(self.fc2(x))

		policy = self.policy_head(x)
		value = torch.tanh(self.value_head(x))

		return policy, value


