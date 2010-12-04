/*
 * Copyright (c) 2010, Mentor Graphics Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the <ORGANIZATION> nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
*   FILENAME
*
*       fts_main.c
*
*
*************************************************************************/

#include "fts_defs.h"
#include "support_suite/mcapid_support.h"

extern MCAPI_MUTEX      MCAPID_FTS_Mutex;

/************************************************************************
*
*   FUNCTION
*
*       MCAPI_FTS_Tx_2_23_1
*
*   DESCRIPTION
*
*       Testing mcapi_pktchan_recv_close_i - open rx / close rx
*
*           Node 1 � Create endpoint, open receive side, close receive side
*
*************************************************************************/
MCAPI_THREAD_ENTRY(MCAPI_FTS_Tx_2_23_1)
{
    MCAPID_STRUCT               *mcapi_struct = (MCAPID_STRUCT*)argv;
    size_t                      rx_len;
    mcapi_request_t             request;

    /* Don't let any other test run while this test is running. */
    MCAPI_Obtain_Mutex(&MCAPID_FTS_Mutex);

    /* Open the local endpoint as the receiver. */
    mcapi_open_pktchan_recv_i(&mcapi_struct->pkt_rx_handle,
                              mcapi_struct->local_endp,
                              &mcapi_struct->request,
                              &mcapi_struct->status);

    if (mcapi_struct->status == MCAPI_ENOT_CONNECTED)
    {
        /* Close the receive side. */
        mcapi_packetchan_recv_close_i(mcapi_struct->pkt_rx_handle,
                                      &request, &mcapi_struct->status);

        if (mcapi_struct->status == MCAPI_SUCCESS)
        {
            /* Wait for the connection to close. */
            mcapi_wait(&request, &rx_len, &mcapi_struct->status,
                       MCAPI_FTS_TIMEOUT);
        }

        else
        {
            mcapi_struct->status = -1;
        }
    }

    else
    {
        mcapi_struct->status = -1;
    }

    /* Set the state of the test to completed. */
    mcapi_struct->state = 0;

    /* Allow the next test to run. */
    MCAPI_Release_Mutex(&MCAPID_FTS_Mutex);

} /* MCAPI_FTS_Tx_2_23_1 */

/************************************************************************
*
*   FUNCTION
*
*       MCAPI_FTS_Tx_2_23_2
*
*   DESCRIPTION
*
*       Testing mcapi_pktchan_recv_close_i - close tx side as rx side
*
*           Node 1 � Create endpoint, open send side, close as receive side
*
*************************************************************************/
MCAPI_THREAD_ENTRY(MCAPI_FTS_Tx_2_23_2)
{
    MCAPID_STRUCT               *mcapi_struct = (MCAPID_STRUCT*)argv;
    mcapi_request_t             request;
    mcapi_status_t              status;

    /* Don't let any other test run while this test is running. */
    MCAPI_Obtain_Mutex(&MCAPID_FTS_Mutex);

    /* Open the local endpoint as the sender. */
    mcapi_open_pktchan_send_i(&mcapi_struct->pkt_tx_handle,
                              mcapi_struct->local_endp,
                              &mcapi_struct->request,
                              &mcapi_struct->status);

    if (mcapi_struct->status == MCAPI_ENOT_CONNECTED)
    {
        /* Close the send side as the receive side. */
        mcapi_packetchan_recv_close_i(mcapi_struct->pkt_tx_handle,
                                      &request, &mcapi_struct->status);

        if (mcapi_struct->status == MCAPI_EDIR)
        {
            mcapi_struct->status = MCAPI_SUCCESS;
        }

        else
        {
            mcapi_struct->status = -1;
        }

        /* Cancel the open call. */
        mcapi_cancel(&mcapi_struct->request, &status);
    }

    else
    {
        mcapi_struct->status = -1;
    }

    /* Set the state of the test to completed. */
    mcapi_struct->state = 0;

    /* Allow the next test to run. */
    MCAPI_Release_Mutex(&MCAPID_FTS_Mutex);

} /* MCAPI_FTS_Tx_2_23_2 */

/************************************************************************
*
*   FUNCTION
*
*       MCAPI_FTS_Tx_2_23_3
*
*   DESCRIPTION
*
*       Testing mcapi_pktchan_recv_close_i - connect / open rx / close
*       rx
*
*           Node 0 � Create endpoint
*
*           Node 1 � Create endpoint, get endpoint on Node 0, issue
*                    connection, open receive side, close receive side
*
*************************************************************************/
MCAPI_THREAD_ENTRY(MCAPI_FTS_Tx_2_23_3)
{
    MCAPID_STRUCT               *mcapi_struct = (MCAPID_STRUCT*)argv;
    size_t                      rx_len;
    mcapi_status_t              status;
    mcapi_endpoint_t            tx_endp;
    mcapi_request_t             request;

    /* Don't let any other test run while this test is running. */
    MCAPI_Obtain_Mutex(&MCAPID_FTS_Mutex);

    /* Indicate that a remote endpoint should be created. */
    mcapi_struct->status =
        MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_CREATE_ENDP, 1024,
                               mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

    if (mcapi_struct->status == MCAPI_SUCCESS)
    {
        /* Wait for a response. */
        mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

        /* If the endpoint was created. */
        if (mcapi_struct->status == MCAPI_SUCCESS)
        {
            /* Get the send side endpoint. */
            tx_endp = mcapi_get_endpoint(0, 1024, &mcapi_struct->status);

            if (mcapi_struct->status == MCAPI_SUCCESS)
            {
                /* Connect the two endpoints. */
                mcapi_connect_pktchan_i(tx_endp, mcapi_struct->local_endp,
                                        &mcapi_struct->request,
                                        &mcapi_struct->status);

                if (mcapi_struct->status == MCAPI_SUCCESS)
                {
                    /* Wait for the connection to open. */
                    mcapi_wait(&mcapi_struct->request, &rx_len, &mcapi_struct->status,
                               MCAPI_FTS_TIMEOUT);

                    /* Open the local endpoint as the receiver. */
                    mcapi_open_pktchan_recv_i(&mcapi_struct->pkt_rx_handle,
                                              mcapi_struct->local_endp,
                                              &mcapi_struct->request,
                                              &mcapi_struct->status);

                    if (mcapi_struct->status == MCAPI_SUCCESS)
                    {
                        /* Close the receive side. */
                        mcapi_packetchan_recv_close_i(mcapi_struct->pkt_rx_handle,
                                                      &request, &mcapi_struct->status);

                        if (mcapi_struct->status == MCAPI_SUCCESS)
                        {
                            /* Wait for the connection to close. */
                            mcapi_wait(&request, &rx_len,
                                       &mcapi_struct->status, MCAPI_FTS_TIMEOUT);
                        }
                    }
                }
            }

            /* Tell the other side to delete the endpoint. */
            status =
                MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_DELETE_ENDP, 1024,
                                       mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

            if (status == MCAPI_SUCCESS)
            {
                /* Wait for the response. */
                status = MCAPID_RX_Mgmt_Response(mcapi_struct);
            }
        }
    }

    /* Set the state of the test to completed. */
    mcapi_struct->state = 0;

    /* Allow the next test to run. */
    MCAPI_Release_Mutex(&MCAPID_FTS_Mutex);

} /* MCAPI_FTS_Tx_2_23_3 */

/************************************************************************
*
*   FUNCTION
*
*       MCAPI_FTS_Tx_2_23_4
*
*   DESCRIPTION
*
*       Testing mcapi_pktchan_recv_close_i - open rx / connect / close
*       rx
*
*           Node 0 � Create endpoint
*
*           Node 1 � Create endpoint, get endpoint on Node 0, open
*                    receive side, connect, close receive side
*
*************************************************************************/
MCAPI_THREAD_ENTRY(MCAPI_FTS_Tx_2_23_4)
{
    MCAPID_STRUCT               *mcapi_struct = (MCAPID_STRUCT*)argv;
    size_t                      rx_len;
    mcapi_status_t              status;
    mcapi_endpoint_t            tx_endp;
    mcapi_request_t             request;

    /* Don't let any other test run while this test is running. */
    MCAPI_Obtain_Mutex(&MCAPID_FTS_Mutex);

    /* Indicate that a remote endpoint should be created. */
    mcapi_struct->status =
        MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_CREATE_ENDP, 1024,
                               mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

    if (mcapi_struct->status == MCAPI_SUCCESS)
    {
        /* Wait for a response. */
        mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

        /* If the endpoint was created. */
        if (mcapi_struct->status == MCAPI_SUCCESS)
        {
            /* Get the send side endpoint. */
            tx_endp = mcapi_get_endpoint(0, 1024, &mcapi_struct->status);

            if (mcapi_struct->status == MCAPI_SUCCESS)
            {
                /* Open the local endpoint as the receiver. */
                mcapi_open_pktchan_recv_i(&mcapi_struct->pkt_rx_handle,
                                          mcapi_struct->local_endp,
                                          &request, &mcapi_struct->status);

                if (mcapi_struct->status == MCAPI_ENOT_CONNECTED)
                {
                    /* Connect the two endpoints. */
                    mcapi_connect_pktchan_i(tx_endp, mcapi_struct->local_endp,
                                            &mcapi_struct->request,
                                            &mcapi_struct->status);

                    if (mcapi_struct->status == MCAPI_SUCCESS)
                    {
                        /* Wait for the connection to open. */
                        mcapi_wait(&mcapi_struct->request, &rx_len, &mcapi_struct->status,
                                   MCAPI_FTS_TIMEOUT);

                        /* Close the receive side. */
                        mcapi_packetchan_recv_close_i(mcapi_struct->pkt_rx_handle,
                                                      &mcapi_struct->request,
                                                      &mcapi_struct->status);

                        if (mcapi_struct->status == MCAPI_SUCCESS)
                        {
                            /* Wait for the connection to close. */
                            mcapi_wait(&mcapi_struct->request, &rx_len,
                                       &mcapi_struct->status, MCAPI_FTS_TIMEOUT);
                        }
                    }
                }
            }

            /* Tell the other side to delete the endpoint. */
            status =
                MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_DELETE_ENDP, 1024,
                                       mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

            if (status == MCAPI_SUCCESS)
            {
                /* Wait for the response. */
                status = MCAPID_RX_Mgmt_Response(mcapi_struct);
            }
        }
    }

    /* Set the state of the test to completed. */
    mcapi_struct->state = 0;

    /* Allow the next test to run. */
    MCAPI_Release_Mutex(&MCAPID_FTS_Mutex);

} /* MCAPI_FTS_Tx_2_23_4 */

/************************************************************************
*
*   FUNCTION
*
*       MCAPI_FTS_Tx_2_23_5
*
*   DESCRIPTION
*
*       Testing mcapi_pktchan_recv_close_i - open rx / connect / open tx
*       / close rx
*
*           Node 0 � Create endpoint, wait for connection, open send side
*
*           Node 1 � Create endpoint, open receive side, get endpoint on
*                    Node 0, issue connection, wait for tx to open, close
*                    receive side
*
*************************************************************************/
MCAPI_THREAD_ENTRY(MCAPI_FTS_Tx_2_23_5)
{
    MCAPID_STRUCT               *mcapi_struct = (MCAPID_STRUCT*)argv;
    size_t                      rx_len;
    mcapi_status_t              status;
    mcapi_endpoint_t            tx_endp, rx_endp;
    mcapi_request_t             request;

    /* Don't let any other test run while this test is running. */
    MCAPI_Obtain_Mutex(&MCAPID_FTS_Mutex);

    /* An additional endpoint is required for this test. */
    rx_endp = mcapi_create_endpoint(MCAPI_PORT_ANY, &mcapi_struct->status);

    if (mcapi_struct->status == MCAPI_SUCCESS)
    {
        /* Indicate that a remote endpoint should be created. */
        mcapi_struct->status =
            MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_CREATE_ENDP, 1024,
                                   mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

        if (mcapi_struct->status == MCAPI_SUCCESS)
        {
            /* Wait for a response. */
            mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

            /* If the endpoint was created. */
            if (mcapi_struct->status == MCAPI_SUCCESS)
            {
                /* Open the local endpoint as the receiver. */
                mcapi_open_pktchan_recv_i(&mcapi_struct->pkt_rx_handle,
                                          rx_endp, &request, &mcapi_struct->status);

                if (mcapi_struct->status == MCAPI_ENOT_CONNECTED)
                {
                    /* Get the send side endpoint. */
                    tx_endp = mcapi_get_endpoint(0, 1024, &mcapi_struct->status);

                    if (mcapi_struct->status == MCAPI_SUCCESS)
                    {
                        /* Connect the two endpoints. */
                        mcapi_connect_pktchan_i(tx_endp, rx_endp,
                                                &mcapi_struct->request,
                                                &mcapi_struct->status);

                        if (mcapi_struct->status == MCAPI_SUCCESS)
                        {
                            /* Wait for the connection to complete. */
                            mcapi_wait(&mcapi_struct->request, &rx_len, &mcapi_struct->status,
                                       MCAPI_FTS_TIMEOUT);

                            /* Indicate that the endpoint should be opened as a sender. */
                            mcapi_struct->status =
                                MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_OPEN_TX_SIDE_PKT,
                                                       1024, mcapi_struct->local_endp, 0,
                                                       MCAPI_DEFAULT_PRIO);

                            if (mcapi_struct->status == MCAPI_SUCCESS)
                            {
                                /* Wait for a response. */
                                mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

                                /* If the send side was opened. */
                                if (mcapi_struct->status == MCAPI_SUCCESS)
                                {
                                    /* Wait for the open to return successfully. */
                                    mcapi_wait(&request, &rx_len,
                                               &mcapi_struct->status, MCAPI_FTS_TIMEOUT);

                                    if (mcapi_struct->status == MCAPI_SUCCESS)
                                    {
                                        /* Close the receive side. */
                                        mcapi_packetchan_recv_close_i(mcapi_struct->pkt_rx_handle,
                                                                      &request,
                                                                      &mcapi_struct->status);

                                        if (mcapi_struct->status == MCAPI_SUCCESS)
                                        {
                                            /* Wait for the connection to close. */
                                            mcapi_wait(&request, &rx_len,
                                                       &mcapi_struct->status, MCAPI_FTS_TIMEOUT);
                                        }
                                    }

                                    /* Tell the other side to close the send side. */
                                    status =
                                        MCAPID_TX_Mgmt_Message(mcapi_struct,
                                                               MCAPID_MGMT_CLOSE_TX_SIDE_PKT,
                                                               1024, mcapi_struct->local_endp,
                                                               0, MCAPI_DEFAULT_PRIO);

                                    if (mcapi_struct->status == MCAPI_SUCCESS)
                                    {
                                        /* Wait for the response. */
                                        status = MCAPID_RX_Mgmt_Response(mcapi_struct);
                                    }
                                }
                            }
                        }
                    }
                }

                /* Tell the other side to delete the endpoint. */
                status =
                    MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_DELETE_ENDP, 1024,
                                           mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

                if (status == MCAPI_SUCCESS)
                {
                    /* Wait for the response. */
                    status = MCAPID_RX_Mgmt_Response(mcapi_struct);
                }
            }
        }

        /* Delete the extra endpoint. */
        mcapi_delete_endpoint(rx_endp, &status);
    }

    /* Set the state of the test to completed. */
    mcapi_struct->state = 0;

    /* Allow the next test to run. */
    MCAPI_Release_Mutex(&MCAPID_FTS_Mutex);

} /* MCAPI_FTS_Tx_2_23_5 */

/************************************************************************
*
*   FUNCTION
*
*       MCAPI_FTS_Tx_2_23_6
*
*   DESCRIPTION
*
*       Testing mcapi_pktchan_recv_close_i - connect / open tx / open rx
*       / close rx
*
*           Node 0 � Create endpoint, wait for connection, open send side
*
*           Node 1 � Create endpoint, get endpoint on Node 0, issue
*                    connection, wait for tx to open, open receive side,
*                    close receive side
*
*************************************************************************/
MCAPI_THREAD_ENTRY(MCAPI_FTS_Tx_2_23_6)
{
    MCAPID_STRUCT               *mcapi_struct = (MCAPID_STRUCT*)argv;
    size_t                      rx_len;
    mcapi_status_t              status;
    mcapi_endpoint_t            tx_endp, rx_endp;
    mcapi_request_t             request;

    /* Don't let any other test run while this test is running. */
    MCAPI_Obtain_Mutex(&MCAPID_FTS_Mutex);

    /* An extra endpoint is required for this test. */
    rx_endp = mcapi_create_endpoint(MCAPI_PORT_ANY, &mcapi_struct->status);

    if (mcapi_struct->status == MCAPI_SUCCESS)
    {
        /* Indicate that a remote endpoint should be created. */
        mcapi_struct->status =
            MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_CREATE_ENDP, 1024,
                                   mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

        if (mcapi_struct->status == MCAPI_SUCCESS)
        {
            /* Wait for a response. */
            mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

            /* If the endpoint was created. */
            if (mcapi_struct->status == MCAPI_SUCCESS)
            {
                /* Get the send side endpoint. */
                tx_endp = mcapi_get_endpoint(0, 1024, &mcapi_struct->status);

                if (mcapi_struct->status == MCAPI_SUCCESS)
                {
                    /* Connect the two endpoints. */
                    mcapi_connect_pktchan_i(tx_endp, rx_endp,
                                            &mcapi_struct->request,
                                            &mcapi_struct->status);

                    if (mcapi_struct->status == MCAPI_SUCCESS)
                    {
                        /* Wait for the connection to open. */
                        mcapi_wait(&mcapi_struct->request, &rx_len, &mcapi_struct->status,
                                   MCAPI_FTS_TIMEOUT);

                        /* Indicate that the endpoint should be opened as a sender. */
                        mcapi_struct->status =
                            MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_OPEN_TX_SIDE_PKT,
                                                   1024, mcapi_struct->local_endp, 0,
                                                   MCAPI_DEFAULT_PRIO);

                        if (mcapi_struct->status == MCAPI_SUCCESS)
                        {
                            /* Wait for a response. */
                            mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

                            /* If the receive side was opened. */
                            if (mcapi_struct->status == MCAPI_SUCCESS)
                            {
                                /* Open the local endpoint as the receiver. */
                                mcapi_open_pktchan_recv_i(&mcapi_struct->pkt_rx_handle,
                                                          rx_endp, &request,
                                                          &mcapi_struct->status);

                                if (mcapi_struct->status == MCAPI_SUCCESS)
                                {
                                    /* Wait for the open to return successfully. */
                                    mcapi_wait(&request, &rx_len,
                                               &mcapi_struct->status, MCAPI_FTS_TIMEOUT);

                                    if (mcapi_struct->status == MCAPI_SUCCESS)
                                    {
                                        /* Close the receive side. */
                                        mcapi_packetchan_recv_close_i(mcapi_struct->pkt_rx_handle,
                                                                      &request,
                                                                      &mcapi_struct->status);

                                        if (mcapi_struct->status == MCAPI_SUCCESS)
                                        {
                                            /* Wait for the connection to close. */
                                            mcapi_wait(&request, &rx_len,
                                                       &mcapi_struct->status, MCAPI_FTS_TIMEOUT);
                                        }
                                    }
                                }

                                /* Tell the other side to close the send side. */
                                status =
                                    MCAPID_TX_Mgmt_Message(mcapi_struct,
                                                           MCAPID_MGMT_CLOSE_TX_SIDE_PKT,
                                                           1024, mcapi_struct->local_endp,
                                                           0, MCAPI_DEFAULT_PRIO);

                                if (status == MCAPI_SUCCESS)
                                {
                                    /* Wait for the response. */
                                    status =
                                        MCAPID_RX_Mgmt_Response(mcapi_struct);
                                }
                            }
                        }
                    }
                }

                /* Tell the other side to delete the endpoint. */
                status =
                    MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_DELETE_ENDP, 1024,
                                           mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

                if (status == MCAPI_SUCCESS)
                {
                    /* Wait for the response. */
                    status = MCAPID_RX_Mgmt_Response(mcapi_struct);
                }
            }
        }

        /* Delete the extra endpoint. */
        mcapi_delete_endpoint(rx_endp, &status);
    }

    /* Set the state of the test to completed. */
    mcapi_struct->state = 0;

    /* Allow the next test to run. */
    MCAPI_Release_Mutex(&MCAPID_FTS_Mutex);

} /* MCAPI_FTS_Tx_2_23_6 */

/************************************************************************
*
*   FUNCTION
*
*       MCAPI_FTS_Tx_2_23_7
*
*   DESCRIPTION
*
*       Testing mcapi_pktchan_recv_close_i - connect / open rx / open tx
*       / close rx
*
*           Node 0 � Create endpoint, wait for connection, open send side
*
*           Node 1 � Create endpoint, get endpoint on Node 0, issue
*                    connection, open receive side, wait for tx to open,
*                    close receive side
*
*************************************************************************/
MCAPI_THREAD_ENTRY(MCAPI_FTS_Tx_2_23_7)
{
    MCAPID_STRUCT               *mcapi_struct = (MCAPID_STRUCT*)argv;
    size_t                      rx_len;
    mcapi_status_t              status;
    mcapi_endpoint_t            tx_endp, rx_endp;
    mcapi_request_t             request;

    /* Don't let any other test run while this test is running. */
    MCAPI_Obtain_Mutex(&MCAPID_FTS_Mutex);

    /* An extra endpoint is required for this test. */
    rx_endp = mcapi_create_endpoint(MCAPI_PORT_ANY, &mcapi_struct->status);

    if (mcapi_struct->status == MCAPI_SUCCESS)
    {
        /* Indicate that a remote endpoint should be created. */
        mcapi_struct->status =
            MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_CREATE_ENDP, 1024,
                                   mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

        if (mcapi_struct->status == MCAPI_SUCCESS)
        {
            /* Wait for a response. */
            mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

            /* If the endpoint was created. */
            if (mcapi_struct->status == MCAPI_SUCCESS)
            {
                /* Get the send side endpoint. */
                tx_endp = mcapi_get_endpoint(0, 1024, &mcapi_struct->status);

                if (mcapi_struct->status == MCAPI_SUCCESS)
                {
                    /* Connect the two endpoints. */
                    mcapi_connect_pktchan_i(tx_endp, rx_endp,
                                            &mcapi_struct->request,
                                            &mcapi_struct->status);

                    if (mcapi_struct->status == MCAPI_SUCCESS)
                    {
                        /* Wait for the connection to open. */
                        mcapi_wait(&mcapi_struct->request, &rx_len, &mcapi_struct->status,
                                   MCAPI_FTS_TIMEOUT);

                        /* Open the local endpoint as the receiver. */
                        mcapi_open_pktchan_recv_i(&mcapi_struct->pkt_rx_handle,
                                                  rx_endp, &request,
                                                  &mcapi_struct->status);

                        if (mcapi_struct->status == MCAPI_SUCCESS)
                        {
                            /* Indicate that the endpoint should be opened as a sender. */
                            mcapi_struct->status =
                                MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_OPEN_TX_SIDE_PKT,
                                                       1024, mcapi_struct->local_endp, 0,
                                                       MCAPI_DEFAULT_PRIO);

                            if (mcapi_struct->status == MCAPI_SUCCESS)
                            {
                                /* Wait for a response. */
                                mcapi_struct->status = MCAPID_RX_Mgmt_Response(mcapi_struct);

                                /* If the receive side was opened. */
                                if (mcapi_struct->status == MCAPI_SUCCESS)
                                {
                                    /* Wait for the open to return successfully. */
                                    mcapi_wait(&request, &rx_len,
                                               &mcapi_struct->status, MCAPI_FTS_TIMEOUT);

                                    if (mcapi_struct->status == MCAPI_SUCCESS)
                                    {
                                        /* Close the receive side. */
                                        mcapi_packetchan_recv_close_i(mcapi_struct->pkt_rx_handle,
                                                                      &request,
                                                                      &mcapi_struct->status);

                                        if (mcapi_struct->status == MCAPI_SUCCESS)
                                        {
                                            /* Wait for the connection to close. */
                                            mcapi_wait(&request, &rx_len,
                                                       &mcapi_struct->status, MCAPI_FTS_TIMEOUT);
                                        }
                                    }

                                    /* Tell the other side to close the send side. */
                                    status =
                                        MCAPID_TX_Mgmt_Message(mcapi_struct,
                                                               MCAPID_MGMT_CLOSE_TX_SIDE_PKT,
                                                               1024, mcapi_struct->local_endp,
                                                               0, MCAPI_DEFAULT_PRIO);

                                    if (status == MCAPI_SUCCESS)
                                    {
                                        /* Wait for the response. */
                                        status = MCAPID_RX_Mgmt_Response(mcapi_struct);
                                    }
                                }
                            }
                        }
                    }
                }

                /* Tell the other side to delete the endpoint. */
                status =
                    MCAPID_TX_Mgmt_Message(mcapi_struct, MCAPID_MGMT_DELETE_ENDP, 1024,
                                           mcapi_struct->local_endp, 0, MCAPI_DEFAULT_PRIO);

                if (status == MCAPI_SUCCESS)
                {
                    /* Wait for the response. */
                    status = MCAPID_RX_Mgmt_Response(mcapi_struct);
                }
            }
        }

        /* Delete the extra endpoint. */
        mcapi_delete_endpoint(rx_endp, &status);
    }

    /* Set the state of the test to completed. */
    mcapi_struct->state = 0;

    /* Allow the next test to run. */
    MCAPI_Release_Mutex(&MCAPID_FTS_Mutex);

} /* MCAPI_FTS_Tx_2_23_7 */
