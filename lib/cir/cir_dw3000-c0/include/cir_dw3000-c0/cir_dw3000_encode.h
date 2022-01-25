/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */


#ifndef _CIR_DW3000_ENCODE_H_
#define _CIR_DW3000_ENCODE_H_

#include <stdlib.h>
#include <stdint.h>
#include <json/json.h>
#include <cir_dw3000/cir_dw3000.h>

void cir_dw3000_encode(struct cir_dw3000_instance * cir, char * name, uint16_t nsize);

#endif