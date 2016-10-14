#include "common.h"

void* in_handler(void* args) {
    int sockfd = (int)args;
    char buffer[4096];
    struct sockaddr_in clin;
    uint32_t clinlen = sizeof(clin);
    PCLIENT client_info = NULL;
    uint8_t* iv;
    int msgsock = -1;    // open a new socket for xxx

    if( getpeername(sockfd, (struct sockaddr*)&clin, &clinlen) < 0 ) {
        LogEvent(LOG_ERROR, "getpeername() failed : %s", strerror(errno));
        close(sockfd);
        pthread_exit((void*)0);
    }

    uint8_t* key = srv_DoHandshake(sockfd, "dh.param", &iv);
    if(!key) {
        LogEvent(LOG_ERROR, "Client Handshake failed");
        close(sockfd);
        //close(msgsock);
        pthread_exit((void*)0);
    }
    LogEvent(LOG_INFO, "Client Handshake complete");
    while(1) {
        // handle transaction here
        uint8_t* packet = srv_ReadPacket(sockfd, MAX_PACKET, key, iv);
        if(!packet) {   // read failure
            LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
            close(sockfd);
            if(client_info) login_NotifyAllLogin(client_info, 1);
            if(msgsock != -1)   close(msgsock);
            if(client_info)     login_DisconnectClient(client_info);
            //close(msgsock);
            pthread_exit((void*)0);
        }
        if(malloc_usable_size(packet) >= sizeof(PACKET)) {
            PPACKET p = (PPACKET)packet, returned;
            uint32_t result;
            if(p->magic == PACKET_MAGIC) {
                switch(p->ptype) {
                    case PTYPE_REGISTER:
                        LogEvent(LOG_INFO, "Register request : (%s,%s)", p->u.regreq.uname, p->u.regreq.passwd);
                        result = protocol_Register(p);
                        returned = protocol_BuildRegisterResult(result, 0);     /* TODO : Add uid query */
                        if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                            LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                close(sockfd);
                            if(client_info) login_NotifyAllLogin(client_info, 1);
                            if(msgsock != -1)   close(msgsock);
                            //close(msgsock);
                            if(client_info)     login_DisconnectClient(client_info);
                            pthread_exit((void*)0);
                        }
                        free(returned);
                        break;
                    case PTYPE_LOGIN:
                        LogEvent(LOG_INFO, "Login request : (%d,%s)", p->u.logreq.uid, p->u.logreq.passwd);
                        result = protocol_Login(p);
                        if(result == 0) {   //login session
                            msgsock = srv_TCPConnectBack(clin.sin_addr.s_addr, p->u.logreq.port);
                            if(msgsock >= 0) {
                                client_info = login_AddClient(msgsock, p->u.logreq.uid, clin.sin_addr.s_addr, ntohs(clin.sin_port), key, iv);
                                if(!client_info) {
                                    LogEvent(LOG_ERROR, "Already Online.");
                                    result = -1;
                                } else {
                                    login_NotifyAllLogin(client_info, 0);   // notify all
                                }
                            } else {
                                LogEvent(LOG_ERROR, "Connect back failed");
                                result = -1;
                            }
                        }
                        LogEvent(LOG_INFO, "Login Result = %d", result);
                        returned = protocol_BuildLoginResult(result, 0);    /* TODO : add session support */
                        if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                            LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                close(sockfd);
                            if(client_info)     login_NotifyAllLogin(client_info, 1);
                            //close(msgsock);
                            if(msgsock != -1)   close(msgsock);
                            if(client_info)     login_DisconnectClient(client_info);
                            pthread_exit((void*)0);
                        }
                        free(returned);
                        break;
                    case PTYPE_MESSAGE:
                        if(!client_info) {  // not connected
                            LogEvent(LOG_ERROR, "Message Req when not logged in");
                            result = -1;
                            returned = protocol_BuildMessageResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        else {
                            LogEvent(LOG_INFO, "Message Req : (%d->%d)", client_info->uid, p->u.msgreq.to_uid);
                            result = protocol_Message(packet, client_info, sockfd, key, iv);
                            if(result != 0) {
                                LogEvent(LOG_ERROR, "Send msg failed");
                            } else {
                                LogEvent(LOG_INFO, "Send msg success");
                            }
                            returned = protocol_BuildMessageResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_SENDFILE:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "SendFile Req when not logged in");
                            result = -1;
                            returned = protocol_BuildSendFileResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            LogEvent(LOG_INFO, "SendFile Req : (%d->%d) (%s:%d)", client_info->uid, p->u.filereq.to_uid, p->u.filereq.filename, p->u.filereq.filesize);
                            result = protocol_SendFile(p, client_info, sockfd);
                            if(result != 0) {
                                LogEvent(LOG_ERROR, "SendFile Req failed");
                            } else {
                                LogEvent(LOG_INFO, "SendFile Req success");
                            }
                            returned = protocol_BuildSendFileResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_GETFILE:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "GetFile Req when not logged in");
                            result = -1;
                            returned = protocol_BuildGetFileResult(result, 0, 0);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            LogEvent(LOG_INFO, "GetFileReq (%d)", client_info->uid);
                            result = protocol_GetFile(packet, client_info);
                            if(result == 0) {
                                PCLIENT target = login_FindOnlineClientByUID(client_info->freq->from_uid);
                                if(!target) {
                                    LogEvent(LOG_ERROR, "Sender (%d) offline", client_info->freq->from_uid);
                                    result = -1;
                                    returned = protocol_BuildGetFileResult(result, 0, 0);
                                } else 
                                    returned = protocol_BuildGetFileResult(result, target->client_ip, FILET_PORT);
                            } else {
                                returned = protocol_BuildGetFileResult(result, 0, 0);
                            }
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_FOPDONE:     // this returns no packet!
                        if(!client_info) {
                            continue;
                        } else {
                            LogEvent(LOG_INFO, "FopDone (%d)", client_info->uid);
                            result = protocol_FopDone(packet);
                            LogEvent(LOG_INFO, "Fop Result = %d", result);
                            login_RemoveFileReq(client_info);
                        }
                        break;
                    case PTYPE_CHANGENAME:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "ChangeName req when not logged in");
                            returned = protocol_BuildCNResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            LogEvent(LOG_INFO, "ChangeNameReq (%d)", client_info->uid);
                            result = protocol_ChangeName(packet, client_info);
                            // username changed notify
                            if(result == 0) {
                                login_NotifyAllChangeName(client_info, p->u.cnreq.username);
                            }
                            returned = protocol_BuildCNResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_GETNAME:
                        LogEvent(LOG_INFO, "GetNameReq");
                        returned = protocol_GetName(packet);
                        if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                            LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                close(sockfd);
                            if(client_info)     login_NotifyAllLogin(client_info, 1);
                            if(msgsock != -1)   close(msgsock);
                            //close(msgsock);
                            if(client_info)     login_DisconnectClient(client_info);
                            pthread_exit((void*)0);
                        }
                        break;
                    case PTYPE_ADDFRIENDREQ:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "AddFriend req when not logged in");
                            returned = protocol_BuildAFResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            result = protocol_AddFriend(p, client_info);
                            
                            returned = protocol_BuildAFResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_BEFRIEND:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "BeFriend req when not logged in");
                            returned = protocol_BuildBFResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            result = protocol_BeFriend(p, client_info);

                            returned = protocol_BuildBFResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_QUERYFRIEND:     // additional packet !
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "QueryFriend req when not logged in");
                            returned = protocol_BuildBFResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            uint32_t fcount, *fptr;
                            result = protocol_QueryFriend(p, client_info, &fcount, &fptr);
                            if(result != 0) {
                                returned = protocol_BuildQFResult(result, 0);
                            } else {
                                returned = protocol_BuildQFResult(0, fcount);
                            }
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                            if(result == 0) {
                                if(srv_WritePacket(sockfd, fptr, sizeof(uint32_t)*fcount, key, iv) < 0) {
                                    LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
                                    close(sockfd);
                                    login_NotifyAllLogin(client_info, 1);
                                    //close(msgsock);
                                    if(msgsock != -1)   close(msgsock);
                                    if(client_info)     login_DisconnectClient(client_info);
                                    pthread_exit((void*)0);
                                }
                            }
                            free(fptr);
                        }
                        break;
                    case PTYPE_DELETEFRIEND:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "DelFriend req when not logged in");
                            returned = protocol_BuildBFResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            result = protocol_DeleteFriend(packet, client_info);
                            returned = protocol_BuildDFResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_CREATEGROUP:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "CreateGroup req when not logged in");
                            returned = protocol_BuildCGResult(-1, 0);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            uint32_t gid;
                            result = protocol_CreateGroup(packet, client_info, sockfd, &gid);

                            returned = protocol_BuildCGResult(result, gid);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_GETGROUPS:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "GetGroups req when not logged in");
                            returned = protocol_BuildGGResult(-1, 0);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            uint32_t groupcnt, *groups;
                            result = protocol_GetGroups(packet, client_info, &groupcnt, &groups);

                            returned = protocol_BuildGGResult(result, groupcnt);    // MEMLEAKAGE!!
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);

                            if(result == 0) {
                                srv_WritePacket(sockfd, groups, sizeof(uint32_t)*groupcnt, key, iv);
                                free(groups);
                            }
                        }
                        break;
                    case PTYPE_GETGROUPUSERS:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "GetGroupUsers req when not logged in");
                            returned = protocol_BuildGGUResult(-1, 0);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            uint32_t usercount, *users;
                            result = protocol_GetGroupUsers(packet, client_info, &usercount, &users);

                            returned = protocol_BuildGGUResult(result, usercount);  // MEMLEAKAGE!!
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                login_NotifyAllLogin(client_info, 1);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }

                            if(result == 0) {
                                srv_WritePacket(sockfd, users, usercount*sizeof(uint32_t), key, iv);
                                free(users);
                            }
                        }
                        break;
                    case PTYPE_GROUPMSG:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "GroupMsg req when not logged in");
                            returned = protocol_BuildGMResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            result = protocol_GroupMessage(packet, client_info, sockfd);

                            returned = protocol_BuildGMResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
                                login_NotifyAllLogin(client_info, 1);
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    case PTYPE_GETAVATAR:
			{
			uint32_t avatar_id;
                        result = protocol_GetAvatar(packet, &avatar_id);
                        
                        returned = protocol_BuildGAResult(result, avatar_id);
                        if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
                                if(client_info) login_NotifyAllLogin(client_info, 1);
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                        }
			}
                        break;
                    case PTYPE_SETAVATAR:
                        if(!client_info) {
                            LogEvent(LOG_ERROR, "SetAvatar req when not logged in");
                            returned = protocol_BuildSAResult(-1);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        } else {
                            result = protocol_SetAvatar(packet, client_info);

                            returned = protocol_BuildSAResult(result);
                            if(srv_WritePacket(sockfd, returned, sizeof(PACKET), key, iv) < 0) {
                                LogEvent(LOG_WARNING, "Client (%s:%d) disconnected", inet_ntoa(clin.sin_addr), ntohs(clin.sin_port));
                                login_NotifyAllLogin(client_info, 1);
           	                    close(sockfd);
                                //close(msgsock);
                                if(msgsock != -1)   close(msgsock);
                                if(client_info)     login_DisconnectClient(client_info);
                                pthread_exit((void*)0);
                            }
                            free(returned);
                        }
                        break;
                    default:
                        LogEvent(LOG_WARNING, "Unknown ptype");
                        break;
                }
            } else {
                LogEvent(LOG_WARNING, "Malformed packet");
            }
        } 

        free(packet);
    }
    return NULL;
}

int main(int argc, char* argv[], char* envp[]) {
    LogInit(LOG_PATH);
    LogEvent(LOG_INFO, "Starting LinPop server");
    int sockfd = srv_StartListen(SERVER_PORT);
    if(sockfd < 0)  exit(-1);
    srv_ServeForever(sockfd, in_handler);
    return 0;
}
