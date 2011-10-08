/** @req J1939TP0003 **/
#include "J1939Tp.h"
/** @req J1939TP0172 */
#include "CanIf.h"
/** @req J1939TP0013 */
#include "J1939Tp_Cbk.h"
#include "J1939Tp_Internal.h"
#include "PduR.h"
/** @req J1939TP0015 */
#include "PduR_J1939Tp.h"
#include <string.h>
/** @req J1939TP0193 */
#if (defined(USE_DET))
#include "Det.h"
#endif

/* Globally fulfilled requirements */
/** @req J1939TP0123 */
/** @req J1939TP0165 */
/** @req J1939TP0097 */
/** @req J1939TP0019 */
/** @req J1939TP0184 */
/** @req J1939TP0007 */
/** @req J1939TP0152 */
/** @req J1939TP0018 */
/** @req J1939TP0036 */
/** @req J1939TP0155 */
/** @req J1939TP0152 */
/** @req J1939TP0125 */
/** @req J1939TP0174 */
/** @req J1939TP0041 */
/** @req J1939TP0189 */
/** @req J1939TP0192 */
/** @req J1939TP0156 */

/** @req J1939TP0020 */
static J1939Tp_Internal_GlobalStateInfoType globalState = {
		.State = J1939TP_OFF,
};
static const J1939Tp_ConfigType* J1939Tp_ConfigPtr;
static J1939Tp_Internal_ChannelInfoType channelInfos[J1939TP_CHANNEL_COUNT];
static J1939Tp_Internal_TxChannelInfoType txChannelInfos[J1939TP_TX_CHANNEL_COUNT];
static J1939Tp_Internal_RxChannelInfoType rxChannelInfos[J1939TP_RX_CHANNEL_COUNT];
static J1939Tp_Internal_PgInfoType pgInfos[J1939TP_PG_COUNT];

/** @req J1939TP0087 */
void J1939Tp_Init(const J1939Tp_ConfigType* ConfigPtr) {
	#if (J1939TP_DEV_ERROR_DETECT == STD_ON)
	if (globalState.State == J1939TP_ON) {
		/** @req J1939TP0026 */
		J1939Tp_Internal_ReportError(J1939TP_INIT_ID, J1939TP_E_REINIT);
	}
	#endif
	int rxCount = 0;
	int txCount = 0;
	for (int i = 0; i < J1939TP_CHANNEL_COUNT; i++) {
		if (ConfigPtr->Channels[i].Direction == J1939TP_TX) {
			channelInfos[i].ChannelConfPtr = &(ConfigPtr->Channels[i]);
			channelInfos[i].TxState = &(txChannelInfos[txCount]);
			channelInfos[i].TxState->State = J1939TP_TX_IDLE;
			channelInfos[i].RxState = 0;
			txCount++;
		} else if (ConfigPtr->Channels[i].Direction == J1939TP_RX) {
			channelInfos[i].ChannelConfPtr = &(ConfigPtr->Channels[i]);
			channelInfos[i].TxState = 0;
			channelInfos[i].RxState = &(rxChannelInfos[rxCount]);
			channelInfos[i].RxState->State = J1939TP_RX_IDLE;
			rxCount++;
		} else {
			return; // unexpected
		}
	}
	for (int i = 0; i < J1939TP_PG_COUNT; i++) {
		pgInfos[i].PgConfPtr = &(ConfigPtr->Pgs[i]);
		uint8 channelIndex = ConfigPtr->Pgs[i].Channel - ConfigPtr->Channels;
		pgInfos[i].ChannelInfoPtr = &(channelInfos[channelIndex]);

	}
	J1939Tp_ConfigPtr = ConfigPtr;
	globalState.State = J1939TP_ON; /** @req J1939TP0022 */
}

void J1939Tp_RxIndication(PduIdType RxPduId, PduInfoType* PduInfoPtr) {
	imask_t state;
	Irq_Save(state);
	/** @req J1939TP0030 */
	if (globalState.State == J1939TP_ON) {
		const J1939Tp_RxPduInfoType* RxPduInfo = 0;
		if (J1939Tp_Internal_GetRxPduInfo(RxPduId, &RxPduInfo) == E_OK) {
			J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr = J1939Tp_Internal_GetChannelState(RxPduInfo);
			switch (RxPduInfo->PacketType ) {
				case J1939TP_REVERSE_CM:
					J1939Tp_Internal_RxIndication_ReverseCm(PduInfoPtr,ChannelInfoPtr);
					break;
				case J1939TP_DT:
					J1939Tp_Internal_RxIndication_Dt(PduInfoPtr,ChannelInfoPtr);
					break;
				case J1939TP_CM:
					J1939Tp_Internal_RxIndication_Cm(PduInfoPtr,ChannelInfoPtr);
					break;
				case J1939TP_DIRECT:
					J1939Tp_Internal_RxIndication_Direct(PduInfoPtr,ChannelInfoPtr);
					break;
				default:
					break;
			}
		}
	}
	Irq_Restore(state);
}


void J1939Tp_MainFunction(void) {
	imask_t state;
	Irq_Save(state);
	/** @req J1939TP0030 */
	if (globalState.State == J1939TP_ON) {
		for (int i = 0; i < J1939TP_CHANNEL_COUNT; i++) {
			J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr = &(channelInfos[i]);
			const J1939Tp_ChannelType* Channel = ChannelInfoPtr->ChannelConfPtr;
			J1939Tp_Internal_TimerStatusType timer = J1939TP_NOT_EXPIRED;
			switch (Channel->Direction) {
				case J1939TP_TX:
					switch (ChannelInfoPtr->TxState->State) {
						case J1939TP_TX_WAIT_DIRECT_SEND_CANIF_CONFIRM:
						case J1939TP_TX_WAITING_FOR_CTS:
						case J1939TP_TX_WAITING_FOR_END_OF_MSG_ACK:
						case J1939TP_TX_WAIT_DT_CANIF_CONFIRM:
						case J1939TP_TX_WAIT_RTS_CANIF_CONFIRM:
							timer = J1939Tp_Internal_IncAndCheckTimer(&(ChannelInfoPtr->TxState->TimerInfo));
							if (timer == J1939TP_EXPIRED) {
								timer = J1939TP_NOT_EXPIRED;
								ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
								if (ChannelInfoPtr->TxState->CurrentPgPtr != 0) {
									J1939Tp_Internal_SendConnectionAbort(Channel->CmNPdu,ChannelInfoPtr->TxState->CurrentPgPtr->Pgn);
									PduR_J1939TpTxConfirmation(ChannelInfoPtr->TxState->CurrentPgPtr->NSdu,NTFRSLT_E_NOT_OK);
								}
							}
							break;
						case J1939TP_TX_WAITING_FOR_T1_TIMEOUT:
							timer = J1939Tp_Internal_IncAndCheckTimer(&(ChannelInfoPtr->TxState->TimerInfo));
							if (timer == J1939TP_EXPIRED) {
								timer = J1939TP_NOT_EXPIRED;
								ChannelInfoPtr->TxState->State = J1939TP_TX_WAIT_DT_BAM_CANIF_CONFIRM;
								J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_TX_CONF_TIMEOUT);
								if (J1939Tp_Internal_SendDt(ChannelInfoPtr) == E_NOT_OK) {
									ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
									PduR_J1939TpTxConfirmation(ChannelInfoPtr->TxState->CurrentPgPtr->NSdu,NTFRSLT_E_NOT_OK);
									J1939Tp_Internal_SendConnectionAbort(ChannelInfoPtr->ChannelConfPtr->CmNPdu,ChannelInfoPtr->TxState->CurrentPgPtr->Pgn);
								}

							}
							break;
						default:
						break;
					}

					break;;
				case J1939TP_RX:
					switch (ChannelInfoPtr->RxState->State) {
						case J1939TP_RX_WAIT_CTS_CANIF_CONFIRM:
						case J1939TP_RX_RECEIVING_DT:
						case J1939TP_RX_WAIT_ENDOFMSGACK_CANIF_CONFIRM:
							timer = J1939Tp_Internal_IncAndCheckTimer(&(ChannelInfoPtr->RxState->TimerInfo));
						default:
							break;
					}
					if (timer == J1939TP_EXPIRED) {
						timer = J1939TP_NOT_EXPIRED;
						ChannelInfoPtr->RxState->State = J1939TP_RX_IDLE;
						if (Channel->Protocol == J1939TP_PROTOCOL_CMDT) {
							J1939Tp_Internal_SendConnectionAbort(Channel->FcNPdu,ChannelInfoPtr->RxState->CurrentPgPtr->Pgn);
							PduR_J1939TpRxIndication(ChannelInfoPtr->RxState->CurrentPgPtr->NSdu,NTFRSLT_E_NOT_OK);
						}
					}
				default:
					break;
			}
		}
	}
	Irq_Restore(state);
}
void J1939Tp_TxConfirmation(PduIdType RxPdu) {
	imask_t state;
	Irq_Save(state);
	/** @req J1939TP0030 */
	if (globalState.State == J1939TP_ON) {
		const J1939Tp_RxPduInfoType* RxPduInfo = 0;
		if (J1939Tp_Internal_GetRxPduInfo(RxPdu, &RxPduInfo) == E_OK) {
			const J1939Tp_ChannelType* Channel = J1939Tp_Internal_GetChannel(RxPduInfo);
			J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr = J1939Tp_Internal_GetChannelState(RxPduInfo);
			switch (Channel->Direction) {
				case J1939TP_TX:
					J1939Tp_Internal_TxConfirmation_TxChannel(ChannelInfoPtr, RxPduInfo);
					break;
				case J1939TP_RX:
					J1939Tp_Internal_TxConfirmation_RxChannel(ChannelInfoPtr, RxPduInfo);
					break;
				default:
					break;
			}
		}
	}
	Irq_Restore(state);
}
static inline const J1939Tp_ChannelType* J1939Tp_Internal_GetChannel(const J1939Tp_RxPduInfoType* RxPduInfo) {
	return &(J1939Tp_ConfigPtr->Channels[RxPduInfo->ChannelIndex]);
}
static inline J1939Tp_Internal_ChannelInfoType* J1939Tp_Internal_GetChannelState(const J1939Tp_RxPduInfoType* RxPduInfo) {
	return &(channelInfos[RxPduInfo->ChannelIndex]);
}
static inline Std_ReturnType J1939Tp_Internal_ValidatePacketType(const J1939Tp_RxPduInfoType* RxPduInfo) {
	const J1939Tp_ChannelType* ChannelPtr = J1939Tp_Internal_GetChannel(RxPduInfo);
	switch (RxPduInfo->PacketType) {
		case J1939TP_REVERSE_CM:
			if (ChannelPtr->Direction != J1939TP_TX ) {
				return E_NOT_OK;
			}
			break;
		case J1939TP_DT:
			if (ChannelPtr->Direction != J1939TP_RX) {
				return E_NOT_OK;
			}
			break;
		case J1939TP_CM:
			if (ChannelPtr->Direction != J1939TP_RX) {
				return E_NOT_OK;
			}
			break;
		default:
			return E_NOT_OK;
	}
	return E_OK;
}
static inline void J1939Tp_Internal_RxIndication_Direct(PduInfoType* PduInfoPtr, J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr) {
	if (PduInfoPtr->SduLength != DIRECT_TRANSMIT_SIZE) {
		return;
	}
	const J1939Tp_PgType* Pg = ChannelInfoPtr->RxState->CurrentPgPtr;
	PduInfoType* rxPduInfo;
	BufReq_ReturnType r = PduR_J1939TpProvideRxBuffer(Pg->NSdu, DIRECT_TRANSMIT_SIZE, &rxPduInfo);
	if (r == BUFREQ_OK) {
		memcpy(rxPduInfo->SduDataPtr,&(PduInfoPtr),DIRECT_TRANSMIT_SIZE);
		PduR_J1939TpRxIndication(Pg->NSdu,NTFRSLT_OK);
		ChannelInfoPtr->RxState->State = J1939TP_RX_IDLE;
	}
}
static inline uint8 J1939Tp_Internal_GetDtPacketsInNextCts(uint8 receivedDtPackets, uint8 totalDtPacketsToReceive) {
	uint8 packetsLeft = totalDtPacketsToReceive - receivedDtPackets;
	if (packetsLeft < (J1939TP_PACKETS_PER_BLOCK)) {
		return packetsLeft;
	} else {
		return J1939TP_PACKETS_PER_BLOCK;
	}
}
static inline void J1939Tp_Internal_RxIndication_Dt(PduInfoType* PduInfoPtr, J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr) {
	PduInfoType* rxPduInfo;
	if (ChannelInfoPtr->RxState->CurrentPgPtr == 0) {
		return;
	}
	if (ChannelInfoPtr->RxState->State != J1939TP_RX_RECEIVING_DT) {
		return;
	}
	PduIdType PduRSdu = ChannelInfoPtr->RxState->CurrentPgPtr->NSdu;
	uint8 requestSize = DT_DATA_SIZE;
	boolean lastMessage = false;

	ChannelInfoPtr->RxState->ReceivedDtCount++;
	uint8 expectedSeqNumber = ChannelInfoPtr->RxState->ReceivedDtCount;
	uint8 seqNumber = PduInfoPtr->SduDataPtr[DT_BYTE_SEQ_NUM];
	if (seqNumber != expectedSeqNumber) {
		ChannelInfoPtr->RxState->State = J1939TP_RX_IDLE;
		if (ChannelInfoPtr->ChannelConfPtr->Protocol == J1939TP_PROTOCOL_CMDT) {
			J1939Tp_PgnType pgn = ChannelInfoPtr->RxState->CurrentPgPtr->Pgn;
			J1939Tp_Internal_SendConnectionAbort(ChannelInfoPtr->ChannelConfPtr->FcNPdu,pgn);
		}
		return;
	}
	BufReq_ReturnType r = PduR_J1939TpProvideRxBuffer(PduRSdu, requestSize, &rxPduInfo);
	if (r == BUFREQ_OK) {
		memcpy(rxPduInfo->SduDataPtr,&(PduInfoPtr[DT_BYTE_DATA_1]),requestSize);
	}
	if (J1939Tp_Internal_IsLastDtBeforeNextCts(ChannelInfoPtr->RxState)) {
		J1939Tp_PgnType pgn = ChannelInfoPtr->RxState->CurrentPgPtr->Pgn;
		uint8 requestPackets = J1939Tp_Internal_GetDtPacketsInNextCts(ChannelInfoPtr->RxState->ReceivedDtCount,ChannelInfoPtr->RxState->DtToReceiveCount);
		ChannelInfoPtr->RxState->State = J1939TP_RX_WAIT_CTS_CANIF_CONFIRM;
		J1939Tp_Internal_SendCts(ChannelInfoPtr, pgn, ChannelInfoPtr->RxState->ReceivedDtCount+1,requestPackets);
	}
	else if (J1939Tp_Internal_IsLastDt(ChannelInfoPtr->RxState)) {
		lastMessage = true;
		J1939Tp_Internal_DtPayloadSizeType receivedBytes = ChannelInfoPtr->RxState->ReceivedDtCount*DT_DATA_SIZE;
		requestSize = ChannelInfoPtr->RxState->TotalMessageSize - receivedBytes;
		J1939Tp_Internal_SendEndOfMsgAck(ChannelInfoPtr);
		ChannelInfoPtr->RxState->State = J1939TP_RX_IDLE;
		PduR_J1939TpRxIndication(PduRSdu,NTFRSLT_OK);
	} else {
		J1939Tp_Internal_RxSessionStartTimer(ChannelInfoPtr->RxState,J1939TP_T1_TIMEOUT_MS);
	}
}

static inline void J1939Tp_Internal_RxIndication_Cm(PduInfoType* PduInfoPtr, J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr) {
	const J1939Tp_PgType* pg = 0;
	J1939Tp_PgnType pgn = J1939Tp_Internal_GetPgn(&(PduInfoPtr->SduDataPtr[CM_PGN_BYTE_1]));
	uint8 pf = J1939Tp_Internal_GetPf(pgn);
	J1939Tp_ProtocolType protocol = J1939Tp_Internal_GetProtocol(pf);
	if (J1939Tp_Internal_GetPgFromPgn(ChannelInfoPtr->ChannelConfPtr,pgn,&pg) != E_OK) {
		return;
	}
	/** @req J1939TP0173 **/
	if (protocol == J1939TP_PROTOCOL_BAM && pg->Channel->Protocol != protocol) {
		return;
	}
	if (protocol == J1939TP_PROTOCOL_CMDT && pg->Channel->Protocol != protocol) {
		J1939Tp_Internal_SendConnectionAbort(pg->Channel->CmNPdu,pgn);
	}
	uint8 Command = PduInfoPtr->SduDataPtr[CM_BYTE_CONTROL];

	/** @req J1939TP0043**/
	J1939Tp_Internal_DtPayloadSizeType messageSize = 0;
	messageSize = J1939Tp_Internal_GetRtsMessageSize(PduInfoPtr);

	if (Command == RTS_CONTROL_VALUE || Command == BAM_CONTROL_VALUE) {
		ChannelInfoPtr->RxState->ReceivedDtCount = 0;
		ChannelInfoPtr->RxState->DtToReceiveCount = PduInfoPtr->SduDataPtr[BAM_RTS_BYTE_NUM_PACKETS];
		ChannelInfoPtr->RxState->TotalMessageSize = messageSize;
		ChannelInfoPtr->RxState->CurrentPgPtr = pg;
		if (Command == RTS_CONTROL_VALUE) {
			ChannelInfoPtr->RxState->State = J1939TP_RX_WAIT_CTS_CANIF_CONFIRM;
			J1939Tp_Internal_RxSessionStartTimer(ChannelInfoPtr->RxState,J1939TP_TX_CONF_TIMEOUT);
			J1939Tp_Internal_SendCts(ChannelInfoPtr,pgn,CTS_START_SEQ_NUM,J1939TP_PACKETS_PER_BLOCK);

		} else if (Command == BAM_CONTROL_VALUE) {
			J1939Tp_Internal_RxSessionStartTimer(ChannelInfoPtr->RxState,J1939TP_T2_TIMEOUT_MS);
			ChannelInfoPtr->RxState->State = J1939TP_RX_RECEIVING_DT;
		}
	} else if (Command == CONNABORT_BYTE_CONTROL) {
		ChannelInfoPtr->RxState->State = J1939TP_RX_IDLE;
	}

}
static inline void J1939Tp_Internal_RxIndication_ReverseCm(PduInfoType* PduInfoPtr, J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr) {
	const J1939Tp_PgType* pg = 0;
	J1939Tp_PgnType pgn = J1939Tp_Internal_GetPgn(&(PduInfoPtr->SduDataPtr[CM_PGN_BYTE_1]));
	if (J1939Tp_Internal_GetPgFromPgn(ChannelInfoPtr->ChannelConfPtr,pgn,&pg) != E_OK) {
		return;
	}
	uint8 Command = PduInfoPtr->SduDataPtr[CM_BYTE_CONTROL];
	switch (Command) {
		case CTS_CONTROL_VALUE:
			if (ChannelInfoPtr->TxState->State == J1939TP_TX_WAITING_FOR_CTS) {
				uint8 NumPacketsToSend = PduInfoPtr->SduDataPtr[CTS_BYTE_NUM_PACKETS];
				uint8 NextPacket = PduInfoPtr->SduDataPtr[CTS_BYTE_NEXT_PACKET];
				if (NumPacketsToSend == 0) {
					// Receiver wants to keep the connection open but cant receive packets
					/** @req J1939TP0195 */
					J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_T4_TIMEOUT_MS);
				} else if(J1939Tp_Internal_IsDtPacketAlreadySent(NextPacket,ChannelInfoPtr->TxState->SentDtCount)) {
					PduR_J1939TpTxConfirmation(pg->NSdu,NTFRSLT_E_NOT_OK);
					/** @req J1939TP0190 */
					J1939Tp_Internal_SendConnectionAbort(ChannelInfoPtr->ChannelConfPtr->CmNPdu,pgn);
					ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
				} else {
					ChannelInfoPtr->TxState->DtToSendBeforeCtsCount = NumPacketsToSend;
					ChannelInfoPtr->TxState->State = J1939TP_TX_WAIT_DT_CANIF_CONFIRM;
					if (J1939Tp_Internal_SendDt(ChannelInfoPtr) == E_NOT_OK) {
						ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
						PduR_J1939TpTxConfirmation(ChannelInfoPtr->TxState->CurrentPgPtr->NSdu,NTFRSLT_E_NOT_OK);
						J1939Tp_Internal_SendConnectionAbort(ChannelInfoPtr->ChannelConfPtr->CmNPdu,ChannelInfoPtr->TxState->CurrentPgPtr->Pgn);
					} else {
						J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_TX_CONF_TIMEOUT);
					}
				}
			}
			break;
		case ENDOFMSGACK_CONTROL_VALUE:
			if (ChannelInfoPtr->TxState->State == J1939TP_TX_WAITING_FOR_END_OF_MSG_ACK) {
				PduR_J1939TpTxConfirmation(pg->NSdu,NTFRSLT_OK);
				ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
			}
			break;
		case CONNABORT_CONTROL_VALUE:
			PduR_J1939TpTxConfirmation(pg->NSdu,NTFRSLT_E_NOT_OK);
			ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
			break;
		default:
			break;
	}
}

/** @req J1939TP0180 */
Std_ReturnType J1939Tp_ChangeParameterRequest(PduIdType SduId, TPParameterType Parameter, uint16 value) {
	return E_NOT_OK; /** @req J1939TP0181 */
}
static inline void J1939Tp_Internal_TxConfirmation_RxChannel(J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr, const J1939Tp_RxPduInfoType* RxPduInfo) {
	switch (RxPduInfo->PacketType ) {
		case J1939TP_REVERSE_CM:
			if (ChannelInfoPtr->RxState->State == J1939TP_RX_WAIT_CTS_CANIF_CONFIRM) {
				J1939Tp_Internal_RxSessionStartTimer(ChannelInfoPtr->RxState,J1939TP_T2_TIMEOUT_MS);
				ChannelInfoPtr->RxState->State = J1939TP_RX_RECEIVING_DT;
			}
			break;
		default:
			break;
	}
}
static inline boolean J1939Tp_Internal_IsDtPacketAlreadySent(uint8 nextPacket, uint8 totalPacketsSent) {
	return nextPacket < totalPacketsSent;
}
static inline Std_ReturnType J1939Tp_Internal_GetRxPduInfo(PduIdType RxPdu,const const J1939Tp_RxPduInfoType** RxPduInfo) {
	if (RxPdu < J1939TP_RX_PDU_COUNT) {
		*RxPduInfo = &(J1939Tp_ConfigPtr->RxPdus[RxPdu]);
		return E_OK;
	} else {
		return E_NOT_OK;
	}

}
static inline Std_ReturnType J1939Tp_Internal_GetPgFromPgn(const J1939Tp_ChannelType* channel, J1939Tp_Internal_PgnType Pgn, const J1939Tp_PgType** Pg) {
	for (int i = 0; i < channel->PgCount; i++) {
		if (channel->Pgs[i]->Pgn == Pgn) {
			*Pg = channel->Pgs[i];
			return E_OK;
		}
	}
	return E_NOT_OK;
}


static inline boolean J1939Tp_Internal_IsLastDtBeforeNextCts(J1939Tp_Internal_RxChannelInfoType* rxChannelInfo) {
	boolean finalDt = rxChannelInfo->ReceivedDtCount == rxChannelInfo->DtToReceiveCount;
	boolean sendNewCts = rxChannelInfo->ReceivedDtCount % J1939TP_PACKETS_PER_BLOCK == 0;
	return !finalDt && sendNewCts;


}
static inline boolean J1939Tp_Internal_IsLastDt(J1939Tp_Internal_RxChannelInfoType* rxChannelInfo) {
	return (rxChannelInfo->DtToReceiveCount == rxChannelInfo->ReceivedDtCount);
}
static inline void J1939Tp_Internal_TxConfirmation_TxChannel(J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr, const J1939Tp_RxPduInfoType* RxPduInfo) {
	J1939Tp_Internal_TxChannelStateType State = ChannelInfoPtr->TxState->State;
	switch (RxPduInfo->PacketType ) {
		case J1939TP_REVERSE_CM:
			break;
		case J1939TP_CM:
			if (State == J1939TP_TX_WAIT_RTS_CANIF_CONFIRM) {
				ChannelInfoPtr->TxState->State = J1939TP_TX_WAITING_FOR_CTS;
				J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_T3_TIMEOUT_MS);
			} else if (State == J1939TP_TX_WAIT_BAM_CANIF_CONFIRM) {
				ChannelInfoPtr->TxState->State = J1939TP_TX_WAITING_FOR_T1_TIMEOUT;
				J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_T1_TIMEOUT_MS);
			}
			break;
		case J1939TP_DT:
			if (State == J1939TP_TX_WAIT_DT_CANIF_CONFIRM) {
				ChannelInfoPtr->TxState->SentDtCount++;
				if (J1939Tp_Internal_LastDtSent(ChannelInfoPtr->TxState)) {
					J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_T3_TIMEOUT_MS);
					ChannelInfoPtr->TxState->State = J1939TP_TX_WAITING_FOR_END_OF_MSG_ACK;
				} else if (J1939Tp_Internal_WaitForCts(ChannelInfoPtr->TxState)) {
					J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_T3_TIMEOUT_MS);
					ChannelInfoPtr->TxState->State = J1939TP_TX_WAITING_FOR_CTS;
				} else {
					J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_TX_CONF_TIMEOUT);
					J1939Tp_Internal_SendDt(ChannelInfoPtr);
				}
			}
			else if (State == J1939TP_TX_WAIT_DT_BAM_CANIF_CONFIRM) {
				ChannelInfoPtr->TxState->SentDtCount++;
				if (J1939Tp_Internal_LastDtSent(ChannelInfoPtr->TxState)) {
					ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
					PduR_J1939TpTxConfirmation(ChannelInfoPtr->TxState->CurrentPgPtr->NSdu,NTFRSLT_OK);
				} else {
					ChannelInfoPtr->TxState->State = J1939TP_TX_WAITING_FOR_T1_TIMEOUT;
					J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_T1_TIMEOUT_MS);
				}
			}
			break;
		case J1939TP_DIRECT:
			if (State == J1939TP_TX_WAIT_DIRECT_SEND_CANIF_CONFIRM) {
				ChannelInfoPtr->TxState->State = J1939TP_TX_IDLE;
				PduR_J1939TpTxConfirmation(ChannelInfoPtr->TxState->CurrentPgPtr->NSdu, NTFRSLT_OK);
			}
			break;
		default:
			break;
	}
}
static inline Std_ReturnType J1939Tp_Internal_GetPg(PduIdType TxSduId,J1939Tp_Internal_PgInfoType** PgInfo) {
	if (TxSduId < J1939TP_PG_COUNT) {
		*PgInfo = &(pgInfos[TxSduId]);
		return E_OK;
	}
	return E_NOT_OK;
}
Std_ReturnType J1939Tp_CancelTransmitRequest(PduIdType TxSduId) {
	/** @req J1939TP0179 **/
	J1939Tp_Internal_PgInfoType* PgInfo;
	if (J1939Tp_Internal_GetPg(TxSduId,&PgInfo) == E_NOT_OK) {
		return E_NOT_OK;
	}
	PduR_J1939TpTxConfirmation(PgInfo->PgConfPtr->NSdu,NTFRSLT_E_CANCELATION_NOT_OK);
	return E_OK;
}
Std_ReturnType J1939Tp_CancelReceiveRequest(PduIdType RxSduId) {
	/** @req J1939TP0178 **/
	J1939Tp_Internal_PgInfoType* PgInfo;
	if (J1939Tp_Internal_GetPg(RxSduId,&PgInfo) == E_NOT_OK) {
		return E_NOT_OK;
	}
	PduR_J1939TpTxConfirmation(PgInfo->PgConfPtr->NSdu,NTFRSLT_E_CANCELATION_NOT_OK);
	return E_OK;
}
/** @req J1939TP0096 */
Std_ReturnType J1939Tp_Transmit(PduIdType TxSduId, const PduInfoType* TxInfoPtr) {
	imask_t state;
	Irq_Save(state);
	#if J1939TP_DEV_ERROR_DETECT
	if (globalState.State == J1939TP_OFF) {
		J1939Tp_Internal_ReportError(J1939TP_TRANSMIT_ID,J1939TP_E_UNINIT);
	}
	#endif
	Std_ReturnType r = E_OK;
	/** @req J1939TP0030 */
	if (globalState.State == J1939TP_ON) {
		J1939Tp_Internal_PgInfoType* PgInfo;
		r = J1939Tp_Internal_GetPg(TxSduId,&PgInfo);
		if (r == E_OK) {
			const J1939Tp_PgType* Pg = PgInfo->PgConfPtr;
			J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr = PgInfo->ChannelInfoPtr;
			/** @req J1939TP0101 **/
			if (ChannelInfoPtr->TxState->State != J1939TP_TX_IDLE) {
				r = E_NOT_OK;
			}
			if (r == E_OK) {
				if (TxInfoPtr->SduLength <= 8) { // direct transmit
					PduInfoType* ToSendTxInfoPtr;
					if (PduR_J1939TpProvideTxBuffer(Pg->NSdu, &ToSendTxInfoPtr, TxInfoPtr->SduLength) == BUFREQ_OK) {
						PduIdType CanIfPdu = Pg->DirectNPdu;
						ChannelInfoPtr->TxState->State = J1939TP_TX_WAIT_DIRECT_SEND_CANIF_CONFIRM;
						ChannelInfoPtr->TxState->CurrentPgPtr = Pg;
						CanIf_Transmit(CanIfPdu,ToSendTxInfoPtr);
					} else {
						r = E_NOT_OK;
					}
				} else {
					//uint8 pf = J1939Tp_Internal_GetPf(Pg->Pgn);
					//J1939Tp_ProtocolType protocol = J1939Tp_Internal_GetProtocol(pf);
					switch (ChannelInfoPtr->ChannelConfPtr->Protocol) { /** @req J1939TP0039*/
						case J1939TP_PROTOCOL_BAM:
							ChannelInfoPtr->TxState->State = J1939TP_TX_WAIT_BAM_CANIF_CONFIRM;
							ChannelInfoPtr->TxState->TotalMessageSize = TxInfoPtr->SduLength;
							ChannelInfoPtr->TxState->CurrentPgPtr = Pg;
							ChannelInfoPtr->TxState->SentDtCount = 0;
							J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_TX_CONF_TIMEOUT);
							J1939Tp_Internal_SendBam(ChannelInfoPtr,TxInfoPtr);
							break;
						case J1939TP_PROTOCOL_CMDT:
							ChannelInfoPtr->TxState->State = J1939TP_TX_WAIT_RTS_CANIF_CONFIRM;
							ChannelInfoPtr->TxState->TotalMessageSize = TxInfoPtr->SduLength;
							ChannelInfoPtr->TxState->PduRPdu = TxSduId;
							ChannelInfoPtr->TxState->CurrentPgPtr = Pg;
							ChannelInfoPtr->TxState->SentDtCount = 0;
							J1939Tp_Internal_SendRts(ChannelInfoPtr,TxInfoPtr);
							J1939Tp_Internal_TxSessionStartTimer(ChannelInfoPtr->TxState,J1939TP_TX_CONF_TIMEOUT);
							break;
					}
				}
			}
		}
	}
	Irq_Restore(state);
	return r;
}
static inline void J1939Tp_Internal_SendBam(J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr,const PduInfoType* TxInfoPtr) {
	uint8 cmBamData[BAM_RTS_SIZE];
	cmBamData[BAM_RTS_BYTE_CONTROL] = BAM_CONTROL_VALUE;
	cmBamData[BAM_RTS_BYTE_LENGTH_1] = (uint8)(TxInfoPtr->SduLength & 0x00FF);
	cmBamData[BAM_RTS_BYTE_LENGTH_2] = (uint8)(TxInfoPtr->SduLength & 0xFF00);

	J1939Tp_PgnType pgn = ChannelInfoPtr->TxState->CurrentPgPtr->Pgn;
	cmBamData[BAM_RTS_BYTE_NUM_PACKETS] = J1939TP_Internal_GetNumDtPacketsToSend(TxInfoPtr->SduLength);
	cmBamData[BAM_RTS_BYTE_SAE_ASSIGN] = 0xFF;
	J1939Tp_Internal_SetPgn(&(cmBamData[BAM_RTS_BYTE_PGN_1]),pgn);

	PduInfoType cmBamPdu;
	cmBamPdu.SduLength = BAM_RTS_SIZE;
	cmBamPdu.SduDataPtr = cmBamData;

	CanIf_Transmit(ChannelInfoPtr->ChannelConfPtr->CmNPdu,&cmBamPdu);
}
static inline uint16 J1939Tp_Internal_GetRtsMessageSize(PduInfoType* pduInfo) {
	return (((uint16)pduInfo->SduDataPtr[BAM_RTS_BYTE_LENGTH_1]) << 8) | pduInfo->SduDataPtr[BAM_RTS_BYTE_LENGTH_2];
}

static inline boolean J1939Tp_Internal_WaitForCts(J1939Tp_Internal_TxChannelInfoType* TxChannelState) {
	return TxChannelState->SentDtCount == TxChannelState->DtToSendBeforeCtsCount;
}

static inline uint8 J1939TP_Internal_GetNumDtPacketsToSend(uint16 messageSize) {
	uint8 packetsToSend = 0;
	packetsToSend = messageSize/DT_DATA_SIZE;
	if (messageSize % DT_DATA_SIZE != 0) {
		packetsToSend = packetsToSend + 1;
	}
	return packetsToSend;
}
static inline boolean J1939Tp_Internal_LastDtSent(J1939Tp_Internal_TxChannelInfoType* TxChannelState) {
	return J1939TP_Internal_GetNumDtPacketsToSend(TxChannelState->TotalMessageSize) == TxChannelState->SentDtCount;
}

static inline Std_ReturnType J1939Tp_Internal_ConfGetPg(PduIdType NSduId, const J1939Tp_PgType* Pg) {
	if (NSduId < J1939TP_PG_COUNT) {
		Pg = &(J1939Tp_ConfigPtr->Pgs[NSduId]);
		return E_OK;
	} else {
		return E_NOT_OK;
	}
}

static inline J1939Tp_Internal_TimerStatusType J1939Tp_Internal_IncAndCheckTimer(J1939Tp_Internal_TimerType* TimerInfo) {
	TimerInfo->Timer += J1939TP_MAIN_FUNCTION_PERIOD;
	if (TimerInfo->Timer >= TimerInfo->TimerExpire) {
		return J1939TP_EXPIRED;
	}
	return J1939TP_NOT_EXPIRED;
}
static inline uint8 J1939Tp_Internal_GetPf(J1939Tp_PgnType pgn) {
	return  (uint8)(pgn >> 8);
}
static J1939Tp_ProtocolType J1939Tp_Internal_GetProtocol(uint8 pf) {
	if (pf < 240) {
		return J1939TP_PROTOCOL_CMDT;
	} else {
		return J1939TP_PROTOCOL_BAM;
	}
}


static inline Std_ReturnType J1939Tp_Internal_SendDt(J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr) {
	uint8 requestLength = DT_DATA_SIZE;
	uint8 bytesLeftToSend = ChannelInfoPtr->TxState->TotalMessageSize - ChannelInfoPtr->TxState->SentDtCount * DT_DATA_SIZE;
	if (bytesLeftToSend < DT_DATA_SIZE){
		requestLength = bytesLeftToSend;
	}
	// prepare dt message
	uint8 dtBuffer[DT_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	PduInfoType dtPduInfoBuffer;
	dtPduInfoBuffer.SduLength = DT_SIZE;
	dtPduInfoBuffer.SduDataPtr = dtBuffer;

	BufReq_ReturnType allocateBufferRes;
	PduInfoType* dataPduInfoBuffer;
	PduIdType Pdur_NSdu = ChannelInfoPtr->TxState->CurrentPgPtr->NSdu;
	allocateBufferRes = PduR_J1939TpProvideTxBuffer(Pdur_NSdu, &dataPduInfoBuffer, requestLength);
	if (allocateBufferRes == BUFREQ_OK) {
		dtPduInfoBuffer.SduDataPtr[DT_BYTE_SEQ_NUM] = ChannelInfoPtr->TxState->SentDtCount+1;
		memcpy(&(dtPduInfoBuffer.SduDataPtr[DT_BYTE_DATA_1]), dataPduInfoBuffer, requestLength);
		PduIdType CanIf_NSdu = ChannelInfoPtr->ChannelConfPtr->DtNPdu;
		CanIf_Transmit(CanIf_NSdu, &dtPduInfoBuffer);
		return E_OK;
	} else {
		return E_NOT_OK;
	}

}

static inline void J1939Tp_Internal_SendRts(J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr, const PduInfoType* TxInfoPtr) {
	uint8 cmRtsData[BAM_RTS_SIZE];
	cmRtsData[BAM_RTS_BYTE_CONTROL] = 16;
	cmRtsData[BAM_RTS_BYTE_LENGTH_1] = (uint8)(TxInfoPtr->SduLength);
	cmRtsData[BAM_RTS_BYTE_LENGTH_2] = (uint8)(TxInfoPtr->SduLength >> 8);

	J1939Tp_PgnType pgn = ChannelInfoPtr->TxState->CurrentPgPtr->Pgn;
	cmRtsData[BAM_RTS_BYTE_NUM_PACKETS] = J1939TP_Internal_GetNumDtPacketsToSend(TxInfoPtr->SduLength);
	cmRtsData[BAM_RTS_BYTE_SAE_ASSIGN] = 0xFF;
	J1939Tp_Internal_SetPgn(&(cmRtsData[BAM_RTS_BYTE_PGN_1]),pgn);

	PduInfoType cmRtsPdu;
	cmRtsPdu.SduLength = BAM_RTS_SIZE;
	cmRtsPdu.SduDataPtr = cmRtsData;

	CanIf_Transmit(ChannelInfoPtr->ChannelConfPtr->CmNPdu,&cmRtsPdu);
}

static inline void J1939Tp_Internal_SendEndOfMsgAck(J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr) {
	PduInfoType endofmsgInfo;
	uint8 endofmsgData[ENDOFMSGACK_SIZE];
	endofmsgInfo.SduLength = ENDOFMSGACK_SIZE;
	endofmsgInfo.SduDataPtr = endofmsgData;
	endofmsgData[ENDOFMSGACK_BYTE_CONTROL] = ENDOFMSGACK_CONTROL_VALUE;
	endofmsgData[ENDOFMSGACK_BYTE_TOTAL_MSG_SIZE_1] = ((uint8)(ChannelInfoPtr->RxState->TotalMessageSize)) << 8;
	endofmsgData[ENDOFMSGACK_BYTE_TOTAL_MSG_SIZE_2] = ((uint8)(ChannelInfoPtr->RxState->TotalMessageSize));
	endofmsgData[ENDOFMSGACK_BYTE_NUM_PACKETS] = ChannelInfoPtr->RxState->ReceivedDtCount;
	endofmsgData[ENDOFMSGACK_BYTE_SAE_ASSIGN] = 0xFF;
	J1939Tp_Internal_SetPgn(&(endofmsgData[ENDOFMSGACK_BYTE_PGN_1]),ChannelInfoPtr->RxState->CurrentPgPtr->Pgn);
	PduIdType CmNPdu = ChannelInfoPtr->ChannelConfPtr->FcNPdu;

	CanIf_Transmit(CmNPdu,&endofmsgInfo);
}

/**
 * Send a response to the incoming RTS
 * @param NSduId
 * @param RtsPduInfoPtr needs to be a valid RTS message
 */
static inline void J1939Tp_Internal_SendCts(J1939Tp_Internal_ChannelInfoType* ChannelInfoPtr, J1939Tp_PgnType Pgn, uint8 NextPacketSeqNum,uint8 NumPackets) {
	PduInfoType ctsInfo;
	uint8 ctsData[CTS_SIZE];
	ctsInfo.SduLength = CTS_SIZE;
	ctsInfo.SduDataPtr = ctsData;
	ctsData[CTS_BYTE_CONTROL] = CTS_CONTROL_VALUE;
	ctsData[CTS_BYTE_NUM_PACKETS] = NumPackets;
	ctsData[CTS_BYTE_NEXT_PACKET] = NextPacketSeqNum;
	ctsData[CTS_BYTE_SAE_ASSIGN_1] = 0xFF;
	ctsData[CTS_BYTE_SAE_ASSIGN_2] = 0xFF;
	J1939Tp_Internal_SetPgn(&(ctsData[BAM_RTS_BYTE_PGN_1]),Pgn);

	CanIf_Transmit(ChannelInfoPtr->ChannelConfPtr->FcNPdu,&ctsInfo);

}
static inline void J1939Tp_Internal_SendConnectionAbort(PduIdType CmNPdu, J1939Tp_PgnType Pgn) {
	PduInfoType connAbortInfo;
	uint8 connAbortData[CONNABORT_SIZE];
	connAbortInfo.SduLength = CONNABORT_SIZE;
	connAbortInfo.SduDataPtr = connAbortData;
	connAbortData[CONNABORT_BYTE_CONTROL] = CONNABORT_CONTROL_VALUE;
	connAbortData[CONNABORT_BYTE_SAE_ASSIGN_1] = 0xFF;
	connAbortData[CONNABORT_BYTE_SAE_ASSIGN_2] = 0xFF;
	connAbortData[CONNABORT_BYTE_SAE_ASSIGN_3] = 0xFF;
	connAbortData[CONNABORT_BYTE_SAE_ASSIGN_4] = 0xFF;
	J1939Tp_Internal_SetPgn(&(connAbortData[CONNABORT_BYTE_PGN_1]),Pgn);
	CanIf_Transmit(CmNPdu,&connAbortInfo);
}
static inline void J1939Tp_Internal_TxSessionStartTimer(J1939Tp_Internal_TxChannelInfoType* Tx,uint16 TimerExpire) {
	Tx->TimerInfo.Timer = 0;
	Tx->TimerInfo.TimerExpire = TimerExpire;
}
static inline void J1939Tp_Internal_RxSessionStartTimer(J1939Tp_Internal_RxChannelInfoType* Rx,uint16 TimerExpire) {
	Rx->TimerInfo.Timer = 0;
	Rx->TimerInfo.TimerExpire = TimerExpire;
}
/**
 * set three bytes to a 18 bit pgn value
 * @param PgnBytes must be three uint8 bytes
 * @param pgn
 */
static inline void J1939Tp_Internal_SetPgn(uint8* PgnBytes,J1939Tp_PgnType pgn ) {
	PgnBytes[2] = pgn; /* get first byte */
	PgnBytes[1] = pgn >> 8; /* get next byte */
	PgnBytes[0] = (pgn >> 16) & 0x3; /* get next two bits */
}
/**
 * return a 18 bit pgn value from three bytes
 * @param PgnBytes must be three uint8 bytes
 * @return
 */
static inline J1939Tp_PgnType J1939Tp_Internal_GetPgn(uint8* PgnBytes) {
	J1939Tp_PgnType pgn = 0;
	pgn = ((uint32)PgnBytes[0]) << 16;
	pgn = pgn | (((uint32)PgnBytes[1]) << 8);
	pgn = pgn | ((uint32)PgnBytes[2]);
	return pgn;
}

static inline void J1939Tp_Internal_ReportError(uint8 ApiId, uint8 ErrorId) {
#if (CANSM_DEV_ERROR_DETECT == STD_ON)
	Det_ReportError(MODULE_ID_J1939TP, 0, ApiId, ApiId);
#endif
}

