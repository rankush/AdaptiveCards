import { ApplicationInsights } from '@ms/1ds-analytics-web-js'; //Imported for 1DS telemetry SDK

/**
 * The purpose of this class is to instantiate the 1DS logger which allows events to be sent from adaptive cards 
 */
export class Microsoft1DSLogger {
	analytics: ApplicationInsights;
	constructor(){
		this.analytics = new ApplicationInsights();
		var config = {
		  //This key is specific to the 1DS project that has been built and can be changed out to represent which 1DS pipeline the events are going to
		  instrumentationKey: "ec1a2b0d82c34dd79e5b7c5c6592b53b-8be431e3-9278-4fcc-be8d-281f5376ca36-7052",
		  channelConfiguration:{ 
			  eventsLimitInMem: 50
		  },
		  propertyConfiguration: { 
			  userAgent: "Custom User Agent"
		  },
		  webAnalyticsConfiguration:{
			  autoCapture: {
				//Setting these to false to not capture any autogenerated data
				scroll: false,
				pageView: false,
				onLoad: false,
				onUnload: false,
				click: false,
				resize: false,
				jsError: false
			  }
		  }
		}; 
		//Initialize SDK with the settings from above
		this.analytics.initialize(config, []);
	}
	/**
 	* Send an event that does not contain a correlationID or valueSet
 	*/
	sendLogData(level: string, message: string){
		this.analytics.trackEvent({
			name: "sendLogData",
			data: {
				level: level,
				message: message,
			},
			baseData: {
				//Area to add baseData
			},
		});
	}
	/**
 	* Send an event that does contain a correlationID or valueSet
 	*/
	sendData(event: string, eventSourceName: string, correlationID?: string, valueSet?: object){
		//takes care of situation where there is and isn't a valueSet in the event
		if (valueSet != undefined){
			//Create array of keys and values from the valueSet object and sends an event for each key value pair
			for(const [key, value] of Object.entries(valueSet)){
				this.analytics.trackEvent({
					name: "sendData",
					data: {
						event: event,
						eventSourceName: eventSourceName,
						correlationID: correlationID,
						key: key,
						value: value,
					},
					baseData: {
						//Area to add baseData
					},
				});
			}			
		}
		else {
			this.analytics.trackEvent({
				name: "sendData",
				data: {
					event: event,
					eventSourceName: eventSourceName,
					correlationID: correlationID,
				},
				baseData: {
					//Area to add baseData
				},
			});
		}		
	}
}