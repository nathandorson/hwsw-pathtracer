#!/usr/bin/env python
# coding: utf-8

# In[1]:


from pynq import Overlay
import numpy as np

overlay = Overlay('/home/xilinx/pynq/overlays/raycast/raycast.bit')

get_ipython().run_line_magic('pinfo', 'overlay')


# In[2]:


raycast_ip = overlay.raycast_0
get_ipython().run_line_magic('pinfo', 'raycast_ip')


# In[3]:


dma = overlay.axi_dma
dma_send = overlay.axi_dma.sendchannel
dma_recv = overlay.axi_dma.recvchannel


# In[4]:


raycast_ip.register_map


# In[5]:


# float * 3, float * 3, float * 3, byte
# point    , normal   , unused   , type
shape = np.array([0, 3, 0, 0, 1, 0, 0, 0, 0], dtype=np.float32)
print(shape)
print(shape.tobytes())

shape2 = np.array([0, -2, 0, 0, 1, 0, 0, 0, 0], dtype=np.float32)

overlay.axi_bram_ctrl_0.write(0, shape.tobytes())
overlay.axi_bram_ctrl_0.write(64, shape2.tobytes())


# In[6]:


get_ipython().run_line_magic('pinfo', 'overlay.axi_bram_ctrl_0.read')


# In[12]:


from pynq import allocate

input_buffer = allocate(shape=(16 * 8,), dtype=np.float32) # 16 input rays
output_buffer = allocate(shape=(16 * 4,), dtype=np.uint32) # 16 output rayhits

input_buffer[0] = 1
input_buffer[1] = 0
input_buffer[2] = 0
input_buffer[3] = 0
input_buffer[4] = 1
input_buffer[5] = 0

input_buffer[8] = 2
input_buffer[9] = 0
input_buffer[10] = 0
input_buffer[11] = 0
input_buffer[12] = 1
input_buffer[13] = 0


# In[13]:


START_CONTROL_REGISTER = 0x0
raycast_ip.write(START_CONTROL_REGISTER, 0x01) # needs to be written before every new stream
raycast_ip.register_map
dma_send.transfer(input_buffer)


# In[14]:


import struct

dma_recv.transfer(output_buffer)

output_buffer


# In[17]:


for i in range(len(output_buffer) // 4):
    print(np.frombuffer(struct.pack('<'+'iii', *output_buffer[4*i:(4*i)+3]), dtype=np.float32))
    print(np.frombuffer(struct.pack('<'+'i', output_buffer[4*i+3]), dtype=np.uint32))


# In[11]:


output_buffer[4:7]


# In[ ]:




