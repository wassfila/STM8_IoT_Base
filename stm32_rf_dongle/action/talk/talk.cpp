
#include "talk.h"
#include "protocol.h"
#include "utils.h"

#define MAX_NB_NODES 10

partner_t speakers[MAX_NB_NODES];

void print_status(Serial *ps,uint8_t st)
{
    switch(st)
    {
        case talk::online           : ps->printf("online"); break;
        case talk::idle             : ps->printf("idle"); break;
        case talk::speak            : ps->printf("speak"); break;
        case talk::listen           : ps->printf("listen"); break;
        case talk::requesting       : ps->printf("req"); break;
        case talk::requested_from   : ps->printf("reqfrm"); break;
    }
}

void print_speakers(Serial *ps,uint8_t *data=NULL)
{
    if(data == NULL)
    {
        ps->printf("Own Speakers: ");
        for(int i=0;i<MAX_NB_NODES;i++)
        {
            if(speakers[i].status != talk::offline)
            {
                ps->printf("(%d,%u:",i,speakers[i].id);
                print_status(ps,speakers[i].status);
                ps->printf(") ");
            }
        }
        ps->printf("\r\n");
    }
    else
    {
        ps->printf("Data Speakers: ");
        for(int i=0;i<MAX_NB_NODES;i++)
        {
            if(data[2*i+1] != talk::offline)
            {
                ps->printf("(%d,%u:",i,data[2*i]);
                print_status(ps,data[2*i+1]);
                ps->printf(") ");
            }
        }
        ps->printf("\r\n");
    }
}

talk_node::talk_node(RfMesh* p_mesh,Serial* p_ser,ws2812B *p_led,uint8_t l_id):
                        mesh(p_mesh),
                        ser(p_ser),
                        led(p_led)
{
    node_id = l_id;

    speakers[0].id = node_id;
    speakers[0].status = talk::offline;
    set_status(talk::offline);

    for(int i=1;i<MAX_NB_NODES;i++)
    {
        speakers[i].id = 0;
        speakers[i].status = talk::offline;
    }
}

void talk_node::self_acted()
{
    uint8_t status = speakers[0].status;
	switch(status)
	{
		case talk::idle : 
        {
            uint8_t speaker_i = whos_in_state(talk::speak);
            if(speaker_i)
            {
                set_status(talk::requesting);
                mesh->send_byte(rf::pid::talk,speakers[speaker_i].id,rf::talk::action_request);
                ser->printf("Acted>I'd like to talk but someone is speaking\n");
            }
            else
            {
                set_status(talk::speak);//here do not wait and apply the status
                mesh->broadcast_byte(rf::pid::talk,rf::talk::action_started_speaking);
                ser->printf("Acted>No one was talking so I started\n");
            }
			break;
        }
		case talk::speak : 
        {
            //no one has requested to speak
            set_status(talk::idle);
            mesh->broadcast_byte(rf::pid::talk,rf::talk::action_stopped_speaking);
            ser->printf("Acted>I was speaking and no one is requesting\n");
			break;
        }
		case talk::requested_from : 
        {
            uint8_t requester_i = whos_in_state(talk::requesting);
            //TODO should check the whole list and do some arbitration
            if(requester_i)
            {
                set_status(talk::listen);
                mesh->send_byte(rf::pid::talk,speakers[requester_i].id,rf::talk::action_give_speach);
                ser->printf("Acted>Giving the speach away and start listening\n");
            }
            else
            {
                set_status(talk::idle);
                mesh->broadcast_byte(rf::pid::talk,rf::talk::action_stopped_speaking);
                ser->printf("Acted>Weired: I was requested from but no one is requesting\n");
            }
			break;
        }
		case talk::listen :
        {
            uint8_t speaker_i = whos_in_state(talk::speak);
            if(speaker_i)
            {
                set_status(talk::requesting);
                mesh->send_byte(rf::pid::talk,speakers[speaker_i].id,rf::talk::action_request);
                ser->printf("Acted>I'd like to talk but someone is speaking\n");
            }
            else
            {
                set_status(talk::speak);//here do not wait and apply the status
                mesh->broadcast_byte(rf::pid::talk,rf::talk::action_started_speaking);
                ser->printf("Acted>Weired: I was listening while no one was speaking\n");
                print_speakers(ser);
            }
			break;
        }
		case talk::requesting :
        {
            uint8_t speaker_i = whos_in_state(talk::speak);
            if(speaker_i)
            {
                mesh->send_byte(rf::pid::talk,speakers[speaker_i].id,rf::talk::action_request_abandoned);
                set_status(talk::listen);
                ser->printf("Acted>Changed my mind, don't want to talk\n");
            }
            else
            {
                set_status(talk::idle);//here do not wait and apply the status
                mesh->broadcast_byte(rf::pid::talk,rf::talk::action_request_abandoned);
                ser->printf("Acted>Weired: I was waiting to speak while no one was speaking\n");
            }
			break;
        }
	}
    wait(2);//slow down the converstation
}

//[0] reserved for own state managed with set_status()
//this update start the search in the table from 1
void talk_node::update_node(uint8_t wake_id,uint8_t st)
{
    ser->printf("update_node>(%d->",wake_id);print_status(ser,st);

    bool registred = false;
    for(int i=1;(i<MAX_NB_NODES)&&(!registred);i++)
    {
        if(speakers[i].id == wake_id)
        {
            speakers[i].status = st;
            registred = true;
        }
    }
    if(!registred)//then take the first offline
    {
        for(int i=1;(i<MAX_NB_NODES)&&(!registred);i++)
        {
            if(speakers[i].status == talk::offline)
            {
                speakers[i].id = wake_id;
                speakers[i].status = st;
                registred = true;
            }
        }
    }
    ser->printf(") : ");print_speakers(ser);
}

void talk_node::get_nodes(uint8_t *data)
{
    for(uint8_t i=0;i<MAX_NB_NODES;i++)
    {
        data[i*2] = speakers[i].id;
        data[i*2+1] = speakers[i].status;
    }
}

//which node is in the given state
//first finding is returned
//the id[0]  as reserved for the self will mean here : false
uint8_t talk_node::whos_in_state(uint8_t st)
{
    uint8_t res = 0;
    for(uint8_t i=0;(i<MAX_NB_NODES)&&!res;i++)
    {
        if(speakers[i].status == st)
        {
            res = i;
        }
    }
    return res;
}

void talk_node::send_list(uint8_t target)
{
    uint8_t buffer[32];
    buffer[0] = 5 + 1 + 20;//size
    buffer[1] = 0x71;//Message with ack, ttl = 1
    buffer[2] = rf::pid::talk;
    buffer[3] = node_id;                //src
    buffer[4] = target;
    buffer[5] = rf::talk::list;  //sub-id
    get_nodes(buffer+rf::ind::p2p_payload+1);
    mesh->send_msg(buffer);
}

void talk_node::broadcast(uint8_t *data,uint8_t size)
{
    uint8_t sub_pid = data[rf::ind::bcst_payload];
    switch(sub_pid)
    {
        case rf::talk::wakeup:
        {
            update_node(data[rf::ind::source],talk::idle);
            send_list(data[rf::ind::source]);
            ser->printf("Rx bcast : wake> sent:");
            print_speakers(ser);
        }
        break;
        case rf::talk::action_request:
        {
            update_node(data[rf::ind::source],talk::requesting);
            if(get_status() == talk::speak)
            {
                ser->printf("Rx Talk Bcast>Wrong: Request to transfer speach (should be message)\n");
                set_status(talk::requested_from);
            }
            else
            {
                ser->printf("Rx Talk Bcast>Wrong: Not speaker (and should be message)\n");
                if(get_status() == talk::idle)
                {
                    set_status(talk::listen);
                    ser->printf("As idle, Start Listening\n");
                }
            }
        }
        break;
        case rf::talk::action_started_speaking:
        {
            update_node(data[rf::ind::source],talk::speak);
            if(get_status() == talk::speak)
            {
                ser->printf("Rx Talk Bcast>Interrupted !!!! :( \n");
                set_status(talk::listen);
            }
            else
            {
                ser->printf("Rx Talk Bcast>Someone started speaking\n");
                if(get_status() == talk::idle)
                {
                    set_status(talk::listen);
                    ser->printf("As idle, Start Listening\n");
                }
            }
        }
        break;
        case rf::talk::action_stopped_speaking:
        {
            update_node(data[rf::ind::source],talk::idle);//TODO assumption, he could be listening
            if(get_status() == talk::listen)
            {
                ser->printf("Rx Talk Bcast>Stopped Speaking\n");
                set_status(talk::idle);
            }
            else
            {
                ser->printf("Rx Talk Bcast>Wrong: Someone stopped speaking, I was not listening was :");
                print_status(ser,get_status());ser->printf("\n");
                //TODO recovery
            }
        }
        break;
        case rf::talk::action_request_abandoned:
        {
            update_node(data[rf::ind::source],talk::listen);//Strong assumption that he started listening
            uint8_t requester_i = whos_in_state(talk::requesting);
            if(requester_i)
            {
                set_status(talk::requested_from);//make sure it stays requested_from
                ser->printf("Rx Talk Bcast>One of requesters abandoned\n");
            }
            else
            {
                set_status(talk::speak);
                ser->printf("Rx Talk Bcast>The last requester abandoned\n");
            }
        }
        break;
        default:
        {
            ser->printf("Unhandled Talk Broadcast id : %u\n",sub_pid);
        }
        break;
    }
}

void talk_node::message(uint8_t *data,uint8_t size)
{
    uint8_t sub_pid = data[rf::ind::p2p_payload];
    switch(sub_pid)
    {
        case rf::talk::list:
        {
            ser->printf("Rx Talk Msg list>");
            uint8_t *p_speakers = data+rf::ind::p2p_payload+1;
            print_speakers(ser,p_speakers);
            update_node(p_speakers[0],p_speakers[1]);//id, state of the first element
            if(whos_in_state(talk::speak))
            {
                set_status(talk::listen);
            }
            else
            {
                set_status(talk::idle);
            }
        }
        break;
        case rf::talk::action_request:
        {
            update_node(data[rf::ind::source],talk::requesting);
            if(get_status() == talk::speak)
            {
                ser->printf("Rx Talk Msg>I'm speaking and speach was requested\n");
                set_status(talk::requested_from);
            }
            else
            {
                ser->printf("Rx Talk Msg>Wrong: I'm not speaking\n");
                if(get_status() == talk::idle)
                {
                    set_status(talk::listen);
                    ser->printf("As idle, Start Listening\n");
                }
            }
        }
        break;
        case rf::talk::action_started_speaking:
        {
            update_node(data[rf::ind::source],talk::speak);
            if(get_status() == talk::speak)
            {
                ser->printf("Rx Talk Msg>Interrupted !!!! :( \n");
                set_status(talk::listen);
            }
            else
            {
                ser->printf("Rx Talk Msg>Someone started speaking\n");
                if(get_status() == talk::idle)
                {
                    set_status(talk::listen);
                    ser->printf("As idle, Start Listening\n");
                }
            }
        }
        break;
        case rf::talk::action_give_speach:
        {
            update_node(data[rf::ind::source],talk::listen);//Strong assumption that he started listening
            if(get_status() == talk::requesting)
            {
                ser->printf("Rx Talk Msg>Thanks for the speach right\n");
                set_status(talk::speak);
                //notify the others
                mesh->broadcast_byte(rf::pid::talk,rf::talk::action_started_speaking);
            }
            else
            {
                ser->printf("Rx Talk Msg>Error: I did not ask for speach:");
                print_status(ser,get_status());ser->printf("\n");
            }
        }
        break;
        case rf::talk::action_request_abandoned :
        {
            update_node(data[rf::ind::source],talk::listen);//Strong assumption that he started listening
            uint8_t requester_i = whos_in_state(talk::requesting);
            if(requester_i)
            {
                set_status(talk::requested_from);//make sure it stays requested_from
                ser->printf("Rx Talk Bcast>One of requesters abandoned\n");
            }
            else
            {
                set_status(talk::speak);
                ser->printf("Rx Talk Bcast>The last requester abandoned\n");
            }
        }
        break;
        default:
            ser->printf("Rx Talk Msg>Unknown sub_pid 0x%02X\n",data[rf::ind::p2p_payload]);
        break;
    }
}

void talk_node::set_color(uint8_t r,uint8_t g,uint8_t b)
{
	uint8_t lr,lg,lb;
	led->get(lr,lg,lb);
	if( (lr != r) || (lg != g) || (lb != b) )
	{
		led->set(r,g,b);//Red
	}
}

void talk_node::set_low()		{	set_color(5,5,5);}
void talk_node::set_red()		{	set_color(0x0F,0,0);}
void talk_node::set_green()	    {	set_color(0,0x0F,0);}
void talk_node::set_blue()		{	set_color(0,0,0x0F);}
void talk_node::set_yellow()	{	set_color(0x0A,0x12,0);}
void talk_node::set_orange()	{	set_color(0x12,0x04,0);}


void talk_node::set_status(uint8_t st)
{
    speakers[0].status = st;

	switch(st)
	{
		case talk::offline : 
			set_low();
			break;
		case talk::idle : 
			set_blue();
			break;
		case talk::speak : 
			set_green();
			break;
		case talk::listen : 
			set_red();
			break;
		case talk::requesting : 
			set_orange();
			break;
		case talk::requested_from : 
			set_yellow();
			break;
	}

	ser->printf("Set Status => ");print_status(ser,st);ser->printf("\n");
}

uint8_t talk_node::get_status()
{
    return speakers[0].status;
}

